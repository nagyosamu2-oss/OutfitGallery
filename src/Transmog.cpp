#include "Transmog.h"
#include "EquipmentRemoval.h"
#include "AccessoryToggle.h"
#include "ItemIdentity.h"
// Skin-first staging investigated against Fitting Room (maartenharms),
// GPL-3.0, commit 8717ae7a8fe6185a65721564bac7237ddd411f65.
// See TRANSMOG-SKIN1.md for source references and prototype limitations.
#include "TransmogHookChain.h"
#include "TransmogIdentity.h"
#include "TransmogSave.h"
#include "TransmogRuntime.h"
#include <mutex>
#include <map>
#include <atomic>
#include <memory>

namespace Gallery {
static void RestorePendingTransmog();
namespace {
std::atomic<std::shared_ptr<const TransmogFailure>> transmogFailure{std::make_shared<const TransmogFailure>()};
std::mutex savedLookMutex;
std::map<std::uint32_t,TransmogSave::State> pendingSavedLooks;
std::atomic<bool> restoreArmed{};
std::atomic<unsigned> restoreAttempts{};
std::atomic<std::uint64_t> restoreDue{};
struct Look {
    Preset preset;
    std::vector<RE::TESObjectARMO*> armor;
    std::uint32_t slots{};
    std::uint32_t scope{~0u}; // cumulative controlled slots; full outfit owns all clothing
    RE::TESRace* race{};
    bool female{};
};
struct ActorLook {
    std::uint32_t id{};
    RE::ActorHandle actor;
    std::atomic<std::shared_ptr<const Look>> look;
    std::vector<RE::NiPointer<RE::NiAVObject>> hiddenRealNodes;
    std::atomic<bool> hiddenNodesPending{}, mode{}, autoOwned{};
    std::size_t lastHidden=~std::size_t{};
    // Diagnostic-only counters are actor-local, so another actor cannot consume
    // this follower's trace. No polling or equipment changes are added.
    std::atomic<unsigned> traceSequence{}, tracePasses{}, traceMasks{};
    std::atomic<bool> traceStaged{};
    std::atomic<unsigned> firstPersonTraces{};
    std::atomic<bool> firstPersonStyled{};
    std::uint64_t traceDue{}; // game-thread-only delayed observations
    unsigned traceSamples{};
};
using ActorLooks=std::vector<std::shared_ptr<ActorLook>>;
std::atomic<std::shared_ptr<const ActorLooks>> actors{std::make_shared<const ActorLooks>()};
std::shared_ptr<ActorLook> FindActorLook(std::uint32_t id) {
    const auto snapshot=actors.load();
    for(const auto& state:*snapshot) if(state->id==id) return state;
    return {};
}
// Registry mutations and node flags belong to the game thread. Hook/UI readers
// retain immutable registry snapshots; no engine calls occur under a lock.
std::shared_ptr<ActorLook> EnsureActorLook(RE::Actor* actor) {
    if(auto state=FindActorLook(actor->GetFormID())) return state;
    auto state=std::make_shared<ActorLook>(); state->id=actor->GetFormID();state->actor=actor->CreateRefHandle();
    auto next=std::make_shared<ActorLooks>(*actors.load());next->push_back(state);actors.store(next);return state;
}
bool installed=false;
using VisitFn=void(*)(RE::InventoryChanges*,RE::InventoryChanges::IItemChangeVisitor&);
using StageFn=bool(*)(RE::TESObjectARMO*,RE::TESRace*,void*,bool);
VisitFn originalVisit{};
using MaskFn=std::uint32_t(*)(RE::InventoryChanges*);
MaskFn originalMask{};
std::atomic<bool> logNextHairMask{};
StageFn stageArmor{};
using RefreshFlagFn=void(*)(RE::AIProcess*,std::uint8_t);
using RefreshModelsFn=void(*)(RE::AIProcess*,RE::Actor*);
RefreshFlagFn refreshFlag{};
RefreshModelsFn refreshModels{};
std::uintptr_t visitorTable{};
thread_local bool inPass=false;
std::atomic<std::uint64_t> renderCoverage{};
std::atomic<bool> logNextPass{};
std::atomic<bool> logNextFirstPerson{};
std::uint64_t diagnosticDue{};
unsigned diagnosticSamples{};
std::atomic<unsigned> diagnosticSequence{};
std::atomic<unsigned> followerTraceSequence{};
// Game-thread-only ownership: only undo flags set by this prototype.

void RestoreHiddenNodes(ActorLook& state) {
    for(auto& node:state.hiddenRealNodes) if(node) node->GetFlags().reset(RE::NiAVObject::Flag::kHidden);
    state.hiddenRealNodes.clear();
    state.hiddenNodesPending=false;
}
bool HasAddon(RE::TESObjectARMO* armor,RE::TESObjectARMA* addon) {
    return armor && addon && std::find(armor->armorAddons.begin(),armor->armorAddons.end(),addon)!=armor->armorAddons.end();
}
std::uint32_t ArmorCoverage(RE::TESObjectARMO* armor) {
    if(!armor) return 0;
    auto slots=armor->GetSlotMask().underlying();
    for(auto* addon:armor->armorAddons) if(addon) slots|=addon->GetSlotMask().underlying();
    return slots;
}
bool TakeTrace(std::atomic<unsigned>& remaining) {
    auto count=remaining.load();
    while(count && !remaining.compare_exchange_weak(count,count-1)) {}
    return count!=0;
}
void DumpFollowerBiped(const ActorLook& state,RE::BipedAnim* biped,const char* phase) {
    SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} phase={} biped={:X}",
        state.traceSequence.load(),state.id,phase,reinterpret_cast<std::uintptr_t>(biped));
    if(!biped) return;
    for(unsigned b=0;b<32;++b) {
        const auto& o=biped->objects[b];
        if(!o.item && !o.addon && !o.part && !o.partClone && b!=0 && b!=1 && b!=2 && b!=3 && b!=7 && b!=11 && b!=23) continue;
        auto node=o.partClone;
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} phase={} slot={} item={:08X} type={} addon={:08X} part={} clone={} hidden={} model={}",
            state.traceSequence.load(),state.id,phase,b+30,o.item?o.item->GetFormID():0,
            o.item?static_cast<unsigned>(o.item->GetFormType()):0,o.addon?o.addon->GetFormID():0,
            bool(o.part),bool(node),node && node->GetFlags().any(RE::NiAVObject::Flag::kHidden),
            o.part && o.part->GetModel()?o.part->GetModel():"");
    }
}
void TraceFollowerModels(const ActorLook& state,RE::TESObjectARMO* armor,RE::TESRace* race,bool female,const char* role) {
    if(!armor) return;
    SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} role={} armor={:08X} name={} slots={:08X}",
        state.traceSequence.load(),state.id,role,armor->GetFormID(),armor->GetName(),armor->GetSlotMask().underlying());
    for(auto* addon:armor->armorAddons) if(addon) {
        const auto* third=addon->bipedModels[female?1:0].GetModel();
        const auto* first=addon->bipedModel1stPersons[female?1:0].GetModel();
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} role={} addon={:08X} raceMatch={} slots={:08X} model3p={} model1p={}",
            state.traceSequence.load(),state.id,role,addon->GetFormID(),race && addon->IsValidRace(race),
            addon->GetSlotMask().underlying(),third?third:"",first?first:"");
    }
}
void MaintainFullLook(ActorLook& state) {
    RestoreHiddenNodes(state);
    const auto current=state.look.load();
    auto held=state.actor.get(); auto* player=held.get();
    if(!current || !player || !player->Get3D()) return;
    auto* base=player->GetActorBase();
    auto* race=player->GetRace();
    if(!base || !race || race!=current->race || base->IsFemale()!=current->female) return;
    auto* skin=base->skin?base->skin:race->skin;
    if(!skin) return;
    for(bool firstPerson:{false,true}) {
        if(firstPerson && player!=RE::PlayerCharacter::GetSingleton()) continue;
        if(firstPerson && !state.firstPersonStyled.load()) continue;
        auto* biped=player->GetBiped1(firstPerson).get();
        if(!biped) continue;
        if(firstPerson && biped==player->GetBiped1(false).get()) continue;
        for(unsigned b=0;b<32;++b) {
            if(firstPerson && !(FirstPersonAppearanceMask&(1u<<b))) continue;
            auto& object=biped->objects[b];
            auto* armor=object.item?object.item->As<RE::TESObjectARMO>():nullptr;
            if(armor && RetainAppearanceArmor(armor->GetSlotMask().underlying(),current->scope)) continue;
            const bool style=std::any_of(current->armor.begin(),current->armor.end(),[&](auto* a){return HasAddon(a,object.addon);});
            if(!ShouldHideRealArmor(b,armor!=nullptr,armor && ((armor->GetSlotMask().underlying()&(1u<<9))!=0),
                armor==skin || HasAddon(skin,object.addon),style)) continue;
            auto node=object.partClone;
            if(node && !node->GetFlags().any(RE::NiAVObject::Flag::kHidden)) {
                node->GetFlags().set(RE::NiAVObject::Flag::kHidden);
                state.hiddenRealNodes.push_back(node);
                state.hiddenNodesPending=true;
            }
        }
    }
    if(state.lastHidden!=state.hiddenRealNodes.size()) {
        state.lastHidden=state.hiddenRealNodes.size();
        SKSE::log::info("FULL-LOOK1 hidden real model nodes={}",state.lastHidden);
    }
}
void DumpBiped(RE::BipedAnim* biped,const char* phase) {
    if(!biped) {SKSE::log::info("HAIR-DIAG seq={} phase={} no biped",diagnosticSequence.load(),phase);return;}
    for(unsigned b=0;b<32;++b) {
        const auto& o=biped->objects[b];
        if(!o.item && !o.addon && !o.part && !o.partClone && b!=0 && b!=1 && b!=11 && b!=23) continue;
        SKSE::log::info("HAIR-DIAG seq={} phase={} slot={} item={:08X} addon={:08X} part={} clone={} model={}",
            diagnosticSequence.load(),phase,b+30,o.item?o.item->GetFormID():0,o.addon?o.addon->GetFormID():0,
            bool(o.part),bool(o.partClone),o.part && o.part->GetModel()?o.part->GetModel():"");
        if(b==23) SKSE::log::info("IDENTITY3 phase={} slot53 itemType={} isARMO={} isARMA={}",phase,
            o.item?static_cast<unsigned>(o.item->GetFormType()):0,
            o.item && o.item->As<RE::TESObjectARMO>(),o.item && o.item->As<RE::TESObjectARMA>());
    }
}

std::uint32_t HairMask(RE::InventoryChanges* changes) {
    const auto real=originalMask(changes);
    auto* player=changes && changes->owner?changes->owner->As<RE::Actor>():nullptr;
    const auto state=player?FindActorLook(player->GetFormID()):nullptr;
    const auto current=state?state->look.load():nullptr;
    if(!current) return real;
    auto* base=player->GetActorBase();
    if(!base || player->GetRace()!=current->race || base->IsFemale()!=current->female) return real;
    const auto result=(current->scope&2u)?AppearanceHairMask(real,current->slots):real;
    if(TakeTrace(state->traceMasks))
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} hair-mask real={:08X} appearance={:08X} scope={:08X} result={:08X}",
            state->traceSequence.load(),state->id,real,current->slots,current->scope,result);
    if(logNextHairMask.exchange(false))
        SKSE::log::info("HAIR4 mask real={:08X} appearance={:08X} result={:08X}",real,current->slots,result);
    return result;
}
void Visit(RE::InventoryChanges* changes,RE::InventoryChanges::IItemChangeVisitor& visitor) {
    auto* player=changes && changes->owner?changes->owner->As<RE::Actor>():nullptr;
    const auto state=player?FindActorLook(player->GetFormID()):nullptr;
    // Always pass the original engine visitor through unchanged. Other render
    // hooks can inspect its layout, so a proxy visitor is not interchangeable.
    if(state && state->look.load() && !inPass &&
       *reinterpret_cast<std::uintptr_t*>(&visitor)==visitorTable) {
        const auto* pass=reinterpret_cast<RE::BSTSmartPointer<RE::BipedAnim>*>(reinterpret_cast<std::byte*>(&visitor)+0x18)->get();
        if(logNextPass.load() || logNextFirstPerson.load())
            SKSE::log::info("SKIN-FIRST4 before original visitor third={} first={}",pass==player->GetBiped1(false).get(),pass==player->GetBiped1(true).get());
    }
    originalVisit(changes,visitor);
    const auto current=state?state->look.load():nullptr;
    if(!current || inPass ||
       *reinterpret_cast<std::uintptr_t*>(&visitor)!=visitorTable) {
        
        return;
    }
    auto* base=player->GetActorBase();
    auto* race=player->GetRace();
    // Only accept the engine visitor validated at installation. Its holder is
    // the biped for THIS pass (including first person), not a guessed actor field.
    auto* holder=reinterpret_cast<RE::BSTSmartPointer<RE::BipedAnim>*>(reinterpret_cast<std::byte*>(&visitor)+0x18);
    auto* biped=holder->get();
    const bool tracePass=TakeTrace(state->tracePasses);
    if(tracePass) {
        auto* third=player->GetBiped1(false).get();
        auto* first=player->GetBiped1(true).get();
        const bool isPlayer=player==RE::PlayerCharacter::GetSingleton();
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} pass={:X} third={:X} first={:X} same={} isPlayer={} eligible={} validActor={}",
            state->traceSequence.load(),state->id,reinterpret_cast<std::uintptr_t>(biped),
            reinterpret_cast<std::uintptr_t>(third),reinterpret_cast<std::uintptr_t>(first),third==first,isPlayer,
            ShouldStyleBiped(biped!=nullptr,biped==third,biped==first,isPlayer),
            base && race && race==current->race && base->IsFemale()==current->female && player->HasKeywordString("ActorTypeNPC"));
    }
    if(!base || !race || !biped || race!=current->race || base->IsFemale()!=current->female || !player->HasKeywordString("ActorTypeNPC")) {
        renderCoverage=0;
        if(base && race && (race!=current->race || base->IsFemale()!=current->female)) state->look.store(nullptr);
        return;
    }
    const bool firstPersonPass=ShouldStylePlayerFirstPerson(biped!=nullptr,
        biped==player->GetBiped1(false).get(),biped==player->GetBiped1(true).get(),player==RE::PlayerCharacter::GetSingleton());
    if(firstPersonPass && (!biped->root || !FirstPersonAppearanceSlots(current->scope))) {
        if(TakeTrace(state->firstPersonTraces)) SKSE::log::info("FIRSTPERSON1 actor={:08X} skipped rootReady={} relevantScope={:08X}",
            state->id,biped->root!=nullptr,FirstPersonAppearanceSlots(current->scope));
        return;
    }
    if(!firstPersonPass && !ShouldStyleBiped(biped!=nullptr,
        biped==player->GetBiped1(false).get(),biped==player->GetBiped1(true).get(), player==RE::PlayerCharacter::GetSingleton())) {
        if(logNextFirstPerson.exchange(false)) SKSE::log::info("SKIN-FIRST4 skipped first-person/unknown biped; original equipment retained");
        return;
    }
    const bool traceFirst=firstPersonPass && TakeTrace(state->firstPersonTraces);
    if(traceFirst) {
        SKSE::log::info("FIRSTPERSON1 actor={:08X} begin biped={:X} scope={:08X}; engine visitor preserved; inventory-based identity restore",
            state->id,reinterpret_cast<std::uintptr_t>(biped),current->scope);
        DumpFollowerBiped(*state,biped,"firstperson-before");
    }
    const bool traceFollower=state->traceSequence.load()!=0 && !state->traceStaged.exchange(true);
    if(traceFollower) DumpFollowerBiped(*state,biped,"before-stage");
    if(logNextPass.load()) SKSE::log::info("SKIN-FIRST4 {} pass; global body mask unchanged",firstPersonPass?"first-person":"third-person");
    struct PassGuard {PassGuard(){inPass=true;} ~PassGuard(){inPass=false;}} guard;
    auto* skin=base->skin ? base->skin : race->skin;
    if(!skin) {
        state->look.store(nullptr); renderCoverage=0;
        SKSE::log::warn("Transmog cleared: actor skin unavailable for empty-slot identity");
        return;
    }
    // A secondary body slot may contain an ARMA marker, not worn ARMO.
    // Snapshot worn identities once per eligible rebuild, never by polling.
    std::array<RE::TESObjectARMO*,32> realItems{};
    std::vector<RE::TESObjectARMO*> retainedReal;
    if(changes->entryList) for(auto* entry:*changes->entryList) {
        if(!entry || !entry->object || !entry->IsWorn()) continue;
        auto* armor=entry->object->As<RE::TESObjectARMO>();
        if(!armor) continue;
        const auto mask=armor->GetSlotMask().underlying();
        if(current->scope!=~0u && RetainAppearanceArmor(mask,current->scope) &&
           std::find(retainedReal.begin(),retainedReal.end(),armor)==retainedReal.end()) retainedReal.push_back(armor);
        for(unsigned b=0;b<32;++b)
            if((mask&(1u<<b)) && !realItems[b]) realItems[b]=armor;
    }
    const bool traceStages=logNextPass.load();
    if(traceStages) DumpBiped(biped,"before-skin");
    // Rebuild this actor's own naked skin first, including hands and feet.
    // The engine stages only addons valid for the current race and sex.
    const bool skinAccepted=stageArmor(skin,race,holder,base->IsFemale());
    if(traceFollower) SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} stage skin={:08X} accepted={}; return value alone does not prove attachment",
        state->traceSequence.load(),state->id,skin->GetFormID(),skinAccepted);
    if(traceStages) {
        SKSE::log::info("SKIN-FIRST4 skin={:08X} race={:08X} female={} accepted={}",skin->GetFormID(),race->GetFormID(),base->IsFemale(),skinAccepted);
        DumpBiped(biped,"after-skin");
    }
    // Partial looks rebuild unaffected real clothes after skin, before overrides.
    for(auto* armor:retainedReal) {
        if(firstPersonPass && !FirstPersonAppearanceSlots(ArmorCoverage(armor))) continue;
        stageArmor(armor,race,holder,base->IsFemale());
    }
    for(auto* armor:current->armor) {
        if(firstPersonPass && !FirstPersonAppearanceSlots(ArmorCoverage(armor))) continue;
        const bool accepted=stageArmor(armor,race,holder,base->IsFemale());
        if(traceFirst) {
            SKSE::log::info("FIRSTPERSON1 actor={:08X} stage armor={:08X} accepted={}; inspect final parts, not return alone",state->id,armor->GetFormID(),accepted);
            TraceFollowerModels(*state,armor,race,base->IsFemale(),"firstperson-appearance");
        }
        if(traceFollower) SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} stage armor={:08X} accepted={}",
            state->traceSequence.load(),state->id,armor->GetFormID(),accepted);
        if(traceStages) SKSE::log::info("SKIN-FIRST4 armor={:08X} name={} accepted={}",armor->GetFormID(),armor->GetName(),accepted);
    }
    // Measure only surviving final addons. Skin also needs a non-null owner
    // when the original equipment had no item in that slot.
    std::uint32_t applied{},skinSlots{};
    for(unsigned b=0;b<32;++b) {
        auto* addon=biped->objects[b].addon;
        if(HasAddon(skin,addon)) skinSlots|=1u<<b;
        if(addon && std::any_of(current->armor.begin(),current->armor.end(),[&](auto* a){return HasAddon(a,addon);})) applied|=1u<<b;
    }
    if(traceStages) DumpBiped(biped,"after-appearance-before-identity");
    if(firstPersonPass) {
        applied=FirstPersonAppearanceSlots(applied);
        skinSlots=FirstPersonAppearanceSlots(skinSlots);
        state->firstPersonStyled.store((applied|skinSlots)!=0);
    }
    if(traceFollower) DumpFollowerBiped(*state,biped,"after-stage-before-identity");
    std::uint32_t skinFallback{};
    for(unsigned b=0;b<32;++b) {
        // Restore only staged slots. Preserve the shield and untouched slots.
        // Never restore a secondary-slot ARMA marker as an armor owner.
        if(!((applied|skinSlots)&(1u<<b)) || b==9) continue;
        if(traceStages) {
            auto* prior=biped->objects[b].item;
            SKSE::log::info("IDENTITY3 slot={} stagedItem={:08X} stagedType={} wornARMO={:08X} chosen={:08X} skinFallback={}",
                b+30,prior?prior->GetFormID():0,prior?static_cast<unsigned>(prior->GetFormType()):0,
                realItems[b]?realItems[b]->GetFormID():0,
                realItems[b]?realItems[b]->GetFormID():skin->GetFormID(),!realItems[b]);
        }
        switch(ChooseIdentityRestore(b,applied|skinSlots,realItems[b]!=nullptr)) {
        case IdentityRestore::Original: biped->objects[b].item=realItems[b]; break;
        case IdentityRestore::Skin: biped->objects[b].item=skin; skinFallback|=1u<<b; break;
        case IdentityRestore::Unchanged: break;
        }
    }
    if(biped==player->GetBiped1(false).get())
        renderCoverage.store((std::uint64_t(skinSlots)<<32)|applied);
    if(logNextPass.load()) DumpBiped(biped,"render-postpass");
    if(traceFirst) {
        DumpFollowerBiped(*state,biped,"firstperson-after-identity");
        SKSE::log::info("FIRSTPERSON1 actor={:08X} end appearance={:08X} skin={:08X} fallback={:08X}",state->id,applied,skinSlots,skinFallback);
    }
    if(traceFollower) {
        DumpFollowerBiped(*state,biped,"after-identity");
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} requested={:08X} finalAppearance={:08X} finalSkin={:08X} skin={:08X} fallback={:08X}",
            state->traceSequence.load(),state->id,current->slots,applied,skinSlots,skin->GetFormID(),skinFallback);
    }
    if(logNextPass.exchange(false)) SKSE::log::info("SKIN-FIRST4 requested {:08X}, final appearance {:08X}, final skin {:08X}; skin={:08X} fallback={:08X}",current->slots,applied,skinSlots,skin->GetFormID(),skinFallback);
}
bool ExpectedCall(std::uintptr_t site,std::uintptr_t target) {
    if(*reinterpret_cast<std::uint8_t*>(site)!=0xE8) return false;
    std::int32_t delta{};
    std::memcpy(&delta,reinterpret_cast<void*>(site+1),sizeof(delta));
    return site+5+delta==target;
}
void Refresh(RE::Actor* player) {
    if(!player || !player->Get3D()) return;
    auto* process=player->GetActorRuntimeData().currentProcess;
    if(!process) return;
    if(!refreshFlag || !refreshModels) return;
    SKSE::log::info("SKIN-FIRST4 equipment refresh begin active={}",TransmogActive(player->GetFormID()));
    refreshFlag(process,1);
    refreshModels(process,player);
    SKSE::log::info("SKIN-FIRST4 equipment refresh completed");
}
}
bool InstallTransmog() {
    TransmogFailure failure;
    const auto runtime=LocateTransmogRuntime(failure);
    if(!runtime) {
        transmogFailure.store(std::make_shared<const TransmogFailure>(std::move(failure)));
        return false;
    }
    visitorTable=runtime->visitorTable;
    stageArmor=reinterpret_cast<StageFn>(runtime->stage);
    refreshFlag=reinterpret_cast<RefreshFlagFn>(runtime->refreshFlag);
    refreshModels=reinterpret_cast<RefreshModelsFn>(runtime->refreshModels);
    SKSE::AllocTrampoline(64);
    // Preserve BOTH previous callees, including foreign module trampolines.
    // An inline jump has no ordinary call return address and is never accepted.
    if(!ExpectedCall(runtime->visitSite,runtime->visitCall) || !ExpectedCall(runtime->maskSite,runtime->maskCall)) {
        failure={TransmogFailureCode::Changed,false,{},"CALL changed immediately before installation"};
        transmogFailure.store(std::make_shared<const TransmogFailure>(failure));
        SKSE::log::warn("RUNTIME4 transmog unavailable: {}",DescribeTransmogFailure(failure,false));
        return false;
    }
    originalVisit=reinterpret_cast<VisitFn>(SKSE::GetTrampoline().write_call<5>(runtime->visitSite,Visit));
    originalMask=reinterpret_cast<MaskFn>(SKSE::GetTrampoline().write_call<5>(runtime->maskSite,HairMask));
    installed=true;
    SKSE::log::info("RUNTIME4 transmog hooks installed on {}; hair={} renderer={}; original visitor and previous callees preserved",
        REL::Module::get().version().string(),runtime->maskModule,runtime->visitModule);
    return true;
}
bool TransmogAvailable(){return installed;}
std::string TransmogUnavailableReason(bool japanese){return DescribeTransmogFailure(*transmogFailure.load(),japanese);}
bool TransmogActive(std::uint32_t id){auto state=FindActorLook(id);return state && bool(state->look.load());}
bool FollowerTransmogMode(std::uint32_t id){auto state=FindActorLook(id);return state && state->mode.load();}
void SetFollowerTransmogMode(RE::Actor* actor,bool enabled){if(actor) EnsureActorLook(actor)->mode=enabled;}
void SetTransmogAutoOwned(RE::Actor* actor,bool owned){if(actor) EnsureActorLook(actor)->autoOwned=owned;}
std::vector<std::uint32_t> TransmogAutoOwnedActors(){std::vector<std::uint32_t> ids;const auto snapshot=actors.load();for(auto& state:*snapshot) if(state->autoOwned) ids.push_back(state->id);return ids;}
bool TransmogMaintenanceDue(){if(restoreArmed && restoreAttempts && GetTickCount64()>=restoreDue) return true;const auto snapshot=actors.load();for(auto& state:*snapshot) if(state->look.load() || state->hiddenNodesPending) return true;return false;}
void BeginTransmogDiagnostic(const Preset& preset,bool transmog) {
    ++diagnosticSequence;
    diagnosticSamples=2;diagnosticDue=GetTickCount64()+1500;
    auto* player=RE::PlayerCharacter::GetSingleton();
    if(!player || !player->GetActorBase()) {diagnosticSamples=0;return;}
    auto* race=player->GetRace();
    SKSE::log::info("HAIR-DIAG seq={} mode={} preset={} race={:08X} female={}",diagnosticSequence.load(),transmog?"transmog":"normal",preset.name,race?race->GetFormID():0,player->GetActorBase()->IsFemale());
    auto* handler=RE::TESDataHandler::GetSingleton();
    for(const auto& item:ClothingPreset(preset).items) {
        auto* form=handler?handler->LookupForm(item.localID,item.plugin):nullptr;
        auto* armor=form?form->As<RE::TESObjectARMO>():nullptr;
        if(!armor) continue;
        SKSE::log::info("HAIR-DIAG armor={} plugin={} local={:06X} slots={:08X}",item.name,item.plugin,item.localID,armor->GetSlotMask().underlying());
        for(auto* addon:armor->armorAddons) if(addon) {
            const char* male=addon->bipedModels[0].GetModel();
            const char* female=addon->bipedModels[1].GetModel();
            SKSE::log::info("HAIR-DIAG addon={:08X} raceMatch={} slots={:08X} male={} female={}",addon->GetFormID(),race && addon->IsValidRace(race),addon->GetSlotMask().underlying(),male?male:"",female?female:"");
        }
    }
    DumpBiped(player->GetBiped1(false).get(),"before-3p");
}
void TickTransmogDiagnostic() {
    RestorePendingTransmog();
    const auto snapshot=actors.load();for(auto& state:*snapshot) MaintainFullLook(*state);
    for(const auto& state:*snapshot) {
        if(!state->traceSamples || GetTickCount64()<state->traceDue) continue;
        auto held=state->actor.get(); auto* actor=held.get();
        if(!state->look.load() || !actor || !actor->Get3D()) {state->traceSamples=0;continue;}
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} settled hiddenOwned={} stagedPassSeen={}",
            state->traceSequence.load(),state->id,state->hiddenRealNodes.size(),state->traceStaged.load());
        DumpFollowerBiped(*state,actor->GetBiped1(false).get(),state->traceSamples==2?"settle-1.5s":"settle-4s");
        --state->traceSamples;state->traceDue=GetTickCount64()+2500;
    }
    if(!diagnosticSamples || GetTickCount64()<diagnosticDue) return;
    auto* player=RE::PlayerCharacter::GetSingleton();
    if(!player || !player->Get3D()) {diagnosticSamples=0;return;}
    DumpBiped(player->GetBiped1(false).get(),diagnosticSamples==2?"settle-1.5s-3p":"settle-4s-3p");
    DumpBiped(player->GetBiped1(true).get(),diagnosticSamples==2?"firstperson-settle-1.5s":"firstperson-settle-4s");
    --diagnosticSamples;diagnosticDue=GetTickCount64()+2500;
}
void ApplyTransmog(const Preset& preset,RE::Actor* player) {
    if(!installed) throw std::runtime_error(TransmogUnavailableReason(false));
    const bool partial=preset.slotMask || preset.accessories;
    auto next=std::make_shared<Look>();
    next->preset=ClothingPreset(preset);
    if(!player) player=RE::PlayerCharacter::GetSingleton();
    if(!player || !player->GetActorBase() || !player->GetRace() || !player->HasKeywordString("ActorTypeNPC")) throw std::runtime_error("Transmog prototype: humanoid actor required.");
    const auto state=EnsureActorLook(player);
    next->race=player->GetRace();next->female=player->GetActorBase()->IsFemale();
    if(!player->GetActorBase()->skin && !next->race->skin) throw std::runtime_error("Transmog prototype: actor skin unavailable.");
    auto* handler=RE::TESDataHandler::GetSingleton();
    if(!handler) throw std::runtime_error("Game data unavailable.");
    // Observe even rejected applications, before touching appearance state.
    // An ordinary equip followed by the same transmog request lets this log
    // compare actual attached addons against our preflight without changing
    // the ordinary-equipment path or forcing an incompatible model.
    const auto request=++followerTraceSequence;
    SKSE::log::info("PREFLIGHT-DIAG2 request={} actor={:08X} name={} preset={} race={:08X} female={}",
        request,player->GetFormID(),player->GetName(),preset.name,next->race->GetFormID(),next->female);
    auto* observed=player->GetBiped1(false).get();
    if(observed) for(unsigned b=0;b<32;++b) {
        const auto& o=observed->objects[b];
        if(!o.item && !o.addon && !o.part && !o.partClone) continue;
        SKSE::log::info("PREFLIGHT-DIAG2 request={} actor={:08X} observed slot={} item={:08X} addon={:08X} clone={} hidden={} model={}",
            request,player->GetFormID(),b+30,o.item?o.item->GetFormID():0,o.addon?o.addon->GetFormID():0,
            bool(o.partClone),o.partClone && o.partClone->GetFlags().any(RE::NiAVObject::Flag::kHidden),
            o.part && o.part->GetModel()?o.part->GetModel():"");
    }
    std::vector<Item> visualItems;
    for(const auto& item:next->preset.items) {
        auto* form=ResolveItem(item);
        auto* armor=form?form->As<RE::TESObjectARMO>():nullptr;
        if(!armor) {
            SKSE::log::warn("TRANSMOG10 skipped missing item: {} ({}:{:06X})",item.name,item.plugin,item.localID);
            continue;
        }
        const auto slots=armor->GetSlotMask().underlying();
        if(!slots) {
            SKSE::log::info("FULL-LOOK2 appearance-only skip: {} ({}:{:06X}), no biped slots; real equipment unchanged",item.name,item.plugin,item.localID);
            continue;
        }
        if(slots&(1u<<9)) throw std::runtime_error("Transmog slot 39 (shield) is not supported: "+item.name);
        bool usable=false;
        for(auto* candidate:armor->armorAddons) if(candidate) {
            const auto* male=candidate->bipedModels[0].GetModel();
            const auto* female=candidate->bipedModels[1].GetModel();
            SKSE::log::info("PREFLIGHT-DIAG2 request={} actor={:08X} armor={:08X} name={} addon={:08X} addonRace={:08X} raceMatch={} male={} female={}",
                request,player->GetFormID(),armor->GetFormID(),armor->GetName(),candidate->GetFormID(),
                candidate->race?candidate->race->GetFormID():0,candidate->IsValidRace(next->race),male?male:"",female?female:"");
        }
        for(auto* addon:armor->armorAddons) {
            if(!addon || !addon->IsValidRace(next->race)) continue;
            const auto* maleModel=addon->bipedModels[0].GetModel();
            const auto* femaleModel=addon->bipedModels[1].GetModel();
            if(HasAppearanceModel(next->female,maleModel && *maleModel,femaleModel && *femaleModel)) {
                if(next->female && (!femaleModel || !*femaleModel))
                    SKSE::log::info("MODEL-FALLBACK-DIAG3 request={} actor={:08X} armor={:08X} addon={:08X} male-field candidate={}; engine selects model, race check retained",
                        request,player->GetFormID(),armor->GetFormID(),addon->GetFormID(),maleModel);
                if(addon->GetSlotMask().underlying()&(1u<<9)) throw std::runtime_error("Transmog model uses slot 39 (shield): "+item.name);
                usable=true;
            }
        }
        if(!usable) {
            SKSE::log::warn("PREFLIGHT-DIAG2 request={} actor={:08X} rejected armor={:08X}; no addon passed both race and sex-model checks",
                request,player->GetFormID(),armor->GetFormID());
            SKSE::log::warn("TRANSMOG10 skipped item without race/sex-compatible model: {} ({}:{:06X})",item.name,item.plugin,item.localID);
            continue;
        }
        // Stage in preset order, as normal equipment does. The engine decides
        // which overlapping addons survive; overlap must not reject the outfit.
        if(next->slots&slots) SKSE::log::info("TRANSMOG10 overlapping slots delegated to engine: {} mask={:08X}",item.name,slots);
        if(std::find(next->armor.begin(),next->armor.end(),armor)!=next->armor.end()) continue;
        next->slots|=slots; next->armor.push_back(armor);
        visualItems.push_back(item);
    }
    if(next->armor.empty()) {
        SKSE::log::warn("PREFLIGHT-DIAG2 request={} actor={:08X} all items rejected; previous appearance retained",request,player->GetFormID());
        throw std::runtime_error("No visual armor in this preset; appearance unchanged.");
    }
    next->preset.items=std::move(visualItems);
    if(partial) {
        // Head presets replace their selected scope, accessories replace only
        // the selected visible items' slots. Validation completes before publish.
        const auto replacement=preset.accessories?next->slots:(preset.slotMask|next->slots);
        next->scope=replacement;
        const auto previous=state->look.load();
        if(previous && previous->race==next->race && previous->female==next->female) {
            next->scope|=previous->scope;
            for(auto* armor:previous->armor) {
                const auto mask=armor->GetSlotMask().underlying();
                if(!RetainAppearanceArmor(mask,replacement)) continue;
                next->armor.push_back(armor); next->slots|=mask;
            }
        }
        SKSE::log::info("PARTIAL5 type={} replacement={:08X} cumulative={:08X} appearance={:08X}",
            preset.accessories?"accessories":"head",replacement,next->scope,next->slots);
    }
    RestoreHiddenNodes(*state);
    if(player==RE::PlayerCharacter::GetSingleton()) {state->firstPersonTraces=2;state->firstPersonStyled=false;}
    if(player!=RE::PlayerCharacter::GetSingleton()) {
        state->traceSequence=++followerTraceSequence;
        state->tracePasses=4;state->traceMasks=2;state->traceStaged=false;
        state->traceSamples=2;state->traceDue=GetTickCount64()+1500;
        auto* skin=player->GetActorBase()->skin?player->GetActorBase()->skin:next->race->skin;
        SKSE::log::info("FOLLOWER-DIAG1 seq={} actor={:08X} name={} preset={} race={:08X} female={} scope={:08X} appearance={:08X}",
            state->traceSequence.load(),state->id,player->GetName(),preset.name,next->race->GetFormID(),next->female,next->scope,next->slots);
        DumpFollowerBiped(*state,player->GetBiped1(false).get(),"before-refresh");
        TraceFollowerModels(*state,skin,next->race,next->female,"skin");
        for(auto* armor:next->armor) TraceFollowerModels(*state,armor,next->race,next->female,"appearance");
    }
    state->look.store(next); renderCoverage=0; logNextPass=true; logNextFirstPerson=true; logNextHairMask=true; Refresh(player);
    SKSE::log::info("Transmog applied: {} armor items; inventory unchanged",next->armor.size());
}
std::vector<RemovalItem> SnapshotRemovalAppearance(RE::Actor* actor) {
    std::vector<RemovalItem> rows;
    const auto state=actor?FindActorLook(actor->GetFormID()):nullptr;
    const auto look=state?state->look.load():nullptr;
    if(look) for(auto* armor:look->armor)
        rows.push_back({armor->GetFormID(),armor->GetSlotMask().underlying(),armor->GetName()?armor->GetName():""});
    return rows;
}
bool RemoveEquippedAccessoryAppearance(const Preset& preset,RE::Actor* actor) {
    std::vector<std::uint32_t> ids;
    for(const auto& item:ClothingPreset(preset).items) {
        auto* form=ResolveItem(item);
        auto* armor=form?form->As<RE::TESObjectARMO>():nullptr;
        if(!armor) return false;
        ids.push_back(armor->GetFormID());
    }
    return RemoveAppearanceIDs(actor,ids);
}
namespace {
std::shared_ptr<const Look> removalOriginalLook;
std::uint32_t removalOriginalActor{};
}
void BeginRemovalAppearance(RE::Actor* actor) {
    removalOriginalActor=actor?actor->GetFormID():0;
    const auto state=actor?FindActorLook(actor->GetFormID()):nullptr;
    removalOriginalLook=state?state->look.load():nullptr;
}
bool RestoreRemovalAppearance(RE::Actor* actor,std::uint32_t id) {
    const auto original=removalOriginalLook;
    if(!actor || actor->GetFormID()!=removalOriginalActor || !original) return false;
    const auto state=FindActorLook(actor->GetFormID());const auto current=state?state->look.load():nullptr;
    if(!current || current->race!=original->race || current->female!=original->female ||
        actor->GetRace()!=original->race || !actor->GetActorBase() || actor->GetActorBase()->IsFemale()!=original->female) return false;
    auto found=std::find_if(original->armor.begin(),original->armor.end(),[&](auto* a){return a->GetFormID()==id;});
    if(found==original->armor.end() || RE::TESForm::LookupByID<RE::TESObjectARMO>(id)!=*found) return false;
    auto next=std::make_shared<Look>(*current);
    if(std::find(next->armor.begin(),next->armor.end(),*found)!=next->armor.end()) return true;
    // Keep the original staging order for restored pieces.
    next->armor.clear();next->slots=0;
    for(auto* armor:original->armor)
        if(armor==*found || std::find(current->armor.begin(),current->armor.end(),armor)!=current->armor.end()) {
            next->armor.push_back(armor);next->slots|=armor->GetSlotMask().underlying();
        }
    // A foreign/new appearance is not part of this dialog session.
    for(auto* armor:current->armor) if(std::find(original->armor.begin(),original->armor.end(),armor)==original->armor.end()) return false;
    RestoreHiddenNodes(*state);state->look.store(next);state->autoOwned=false;
    state->firstPersonTraces=2;state->firstPersonStyled=false;
    renderCoverage=0;logNextPass=true;logNextFirstPerson=true;logNextHairMask=true;Refresh(actor);return true;
}
bool RemoveAppearanceIDs(RE::Actor* actor,const std::vector<std::uint32_t>& wanted) {
    if(!actor) return false;
    const auto state=FindActorLook(actor->GetFormID());
    const auto current=state?state->look.load():nullptr;
    if(!current || current->race!=actor->GetRace() || !actor->GetActorBase() ||
        current->female!=actor->GetActorBase()->IsFemale()) return false;
    std::vector<std::uint32_t> visible;
    for(auto* armor:current->armor) visible.push_back(armor->GetFormID());
    if(!AccessoryToggle::Complete(wanted,visible)) return false;
    const auto retained=AccessoryToggle::Remaining(visible,wanted);
    auto next=std::make_shared<Look>(*current);
    next->armor.clear();next->slots=0;next->preset={};
    for(auto* armor:current->armor) if(std::find(retained.begin(),retained.end(),armor->GetFormID())!=retained.end()) {
        next->armor.push_back(armor);next->slots|=armor->GetSlotMask().underlying();
    }
    // Preserve controlled scope, including an empty look: do not reveal real armor.
    RestoreHiddenNodes(*state);
    state->look.store(next);state->autoOwned=false;
    state->firstPersonTraces=2;state->firstPersonStyled=false;
    renderCoverage=0;logNextPass=true;logNextFirstPerson=true;logNextHairMask=true;
    Refresh(actor);
    SKSE::log::info("ACCESSORY-TOGGLE1 appearance removed actor={:08X} count={} remaining={} scope={:08X}",actor->GetFormID(),wanted.size(),next->armor.size(),next->scope);
    return true;
}
void ClearActorTransmog(RE::Actor* actor,bool refresh) {
    if(!actor) return;
    auto state=FindActorLook(actor->GetFormID());if(!state) return;
    RestoreHiddenNodes(*state);state->autoOwned=false;
    const bool had=bool(state->look.exchange(nullptr));
    if(had && refresh) Refresh(actor);
}
void ClearTransmog(bool refresh) {ClearActorTransmog(RE::PlayerCharacter::GetSingleton(),refresh);diagnosticSamples=0;}
void ClearAllTransmog() {
    const auto snapshot=actors.load();for(auto& state:*snapshot) {RestoreHiddenNodes(*state);state->look.store(nullptr);}
    actors.store(std::make_shared<const ActorLooks>());diagnosticSamples=0;
}
void ResetTransmogPersistence() {
    restoreArmed=false;restoreAttempts=0;restoreDue=0;
    {std::scoped_lock lock(savedLookMutex);pendingSavedLooks.clear();}
    RestorePlayerTransmogMode(false,false);
}
void StartTransmogRestore() {
    std::scoped_lock lock(savedLookMutex);
    restoreAttempts=pendingSavedLooks.empty()?0:30;
    restoreDue=0;restoreArmed=!pendingSavedLooks.empty();
}
void SaveTransmog(SKSE::SerializationInterface* s) {
    try {
        std::map<std::uint32_t,TransmogSave::State> records;
        {std::scoped_lock lock(savedLookMutex);records=pendingSavedLooks;}
        const auto snapshot=actors.load();
        for(const auto& state:*snapshot) {
            TransmogSave::State saved;saved.actor=state->id;
            saved.mode=state->id==0x14?PlayerTransmogMode():state->mode.load();
            const auto current=state->look.load();
            if(current) {
                saved.race=current->race->GetFormID();saved.female=current->female;saved.scope=current->scope;
                saved.autoOwned=state->id==0x14?PlayerTransmogAutoOwned():state->autoOwned.load();
                for(auto* armor:current->armor) saved.armor.push_back(armor->GetFormID());
            }
            if(saved.mode || current || saved.actor==0x14) records[saved.actor]=std::move(saved);
        }
        if(!records.contains(0x14)) {TransmogSave::State player;player.actor=0x14;player.mode=PlayerTransmogMode();records.emplace(0x14,std::move(player));}
        unsigned written{};
        for(const auto& [id,saved]:records) {
            if(written>=TransmogSave::MaxActors) {SKSE::log::warn("TRANSMOG-SAVE1 actor limit reached");break;}
            const auto bytes=TransmogSave::Encode(saved);
            if(!s->OpenRecord(TransmogSave::Record,TransmogSave::Version) || !s->WriteRecordData(bytes.data(),static_cast<std::uint32_t>(bytes.size())))
                throw std::runtime_error("Could not write appearance co-save record");
            ++written;
            SKSE::log::info("TRANSMOG-SAVE1 saved actor={:08X} mode={} armor={} scope={:08X}",id,saved.mode,saved.armor.size(),saved.scope);
        }
    }catch(const std::exception& e) {SKSE::log::error("TRANSMOG-SAVE1 save failed: {}",e.what());}
}
bool LoadTransmog(SKSE::SerializationInterface* s,std::uint32_t type,std::uint32_t version,std::uint32_t length) {
    if(type!=TransmogSave::Record) return false;
    try {
        if(version!=TransmogSave::Version || !length || length>TransmogSave::MaxBytes) throw std::runtime_error("Unsupported appearance record header");
        std::string bytes(length,'\0');
        if(s->ReadRecordData(bytes.data(),length)!=length) throw std::runtime_error("Truncated appearance record");
        auto saved=TransmogSave::Decode(bytes);
        const bool hadLook=saved.race!=0;
        if(!TransmogSave::Remap(saved,[&](std::uint32_t old,std::uint32_t& current){return s->ResolveFormID(old,current);}))
            throw std::runtime_error("Unresolved saved actor");
        if(hadLook && !saved.race) SKSE::log::warn("TRANSMOG-SAVE1 missing race/gear; appearance skipped actor={:08X}; mode retained",saved.actor);
        std::scoped_lock lock(savedLookMutex);
        if(pendingSavedLooks.size()>=TransmogSave::MaxActors || pendingSavedLooks.contains(saved.actor)) throw std::runtime_error("Duplicate/excess saved actor");
        SKSE::log::info("TRANSMOG-SAVE1 queued actor={:08X} mode={} armor={}",saved.actor,saved.mode,saved.armor.size());
        pendingSavedLooks.emplace(saved.actor,std::move(saved));
    }catch(const std::exception& e) {SKSE::log::warn("TRANSMOG-SAVE1 load skipped: {}",e.what());}
    return true;
}
static void RestorePendingTransmog() {
    if(!restoreArmed || !restoreAttempts || GetTickCount64()<restoreDue) return;
    --restoreAttempts;restoreDue=GetTickCount64()+1000;
    std::map<std::uint32_t,TransmogSave::State> records;
    {std::scoped_lock lock(savedLookMutex);records.swap(pendingSavedLooks);}
    for(auto& [id,saved]:records) {
        auto* actor=RE::TESForm::LookupByID<RE::Actor>(id);
        if(!actor) {std::scoped_lock lock(savedLookMutex);pendingSavedLooks.emplace(id,std::move(saved));continue;}
        try {
            const auto state=EnsureActorLook(actor);
            state->mode=saved.mode;
            if(id==0x14) RestorePlayerTransmogMode(saved.mode,false);
            if(!saved.race) {SKSE::log::info("TRANSMOG-SAVE1 restored actor={:08X} mode={} without appearance",id,saved.mode);continue;}
            auto* base=actor->GetActorBase();auto* race=actor->GetRace();
            if(!installed || !base || !race || race->GetFormID()!=saved.race || base->IsFemale()!=saved.female ||
               !actor->HasKeywordString("ActorTypeNPC") || (!base->skin && !race->skin)) throw std::runtime_error("Actor race/sex/skin changed or renderer unavailable");
            auto next=std::make_shared<Look>();next->race=race;next->female=saved.female;next->scope=saved.scope;next->preset.name="Restored appearance";
            for(auto form:saved.armor) {
                auto* armor=RE::TESForm::LookupByID<RE::TESObjectARMO>(form);
                if(!armor || !armor->GetSlotMask().underlying() || (armor->GetSlotMask().underlying()&(1u<<9))) throw std::runtime_error("Saved armor missing or unsupported");
                bool usable=false;
                for(auto* addon:armor->armorAddons) if(addon && addon->IsValidRace(race)) {
                    const auto* male=addon->bipedModels[0].GetModel();const auto* female=addon->bipedModels[1].GetModel();
                    if(HasAppearanceModel(saved.female,male && *male,female && *female)) {
                        if(addon->GetSlotMask().underlying()&(1u<<9)) throw std::runtime_error("Saved armor model uses shield slot");
                        usable=true;
                    }
                }
                if(!usable) throw std::runtime_error("Saved armor no longer has a compatible model");
                next->armor.push_back(armor);next->slots|=armor->GetSlotMask().underlying();
            }
            // Publish the exact accumulated look, not a preset-file reference.
            // Unloaded 3D will consume this state on the actor's next worn pass.
            state->look.store(next);state->autoOwned=id!=0x14 && saved.autoOwned;
            if(id==0x14) {RestorePlayerTransmogMode(saved.mode,saved.autoOwned);state->firstPersonTraces=2;}
            SKSE::log::info("TRANSMOG-SAVE1 restored actor={:08X} mode={} armor={} scope={:08X} loaded3D={}",id,saved.mode,next->armor.size(),next->scope,actor->Get3D()!=nullptr);
            Refresh(actor);
        }catch(const std::exception& e) {SKSE::log::warn("TRANSMOG-SAVE1 appearance not restored actor={:08X}: {}; real equipment retained",id,e.what());}
    }
    {std::scoped_lock lock(savedLookMutex);
        if(pendingSavedLooks.empty()) restoreArmed=false;
        else if(!restoreAttempts) {restoreArmed=false;SKSE::log::warn("TRANSMOG-SAVE1 {} actors unavailable after 30 observations; records retained for next save",pendingSavedLooks.size());}
    }
}
}
