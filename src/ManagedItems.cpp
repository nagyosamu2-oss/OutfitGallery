#include "ManagedItems.h"
#include <RE/T/TESUniqueIDChangeEvent.h>
#include "EnchantmentSignature.h"
#include "ManagedLedger.h"
#include "UniqueItemID.h"
#include "GeneratedIdentity.h"
#include "TransferObservation.h"
#include "TransferChain.h"
#include "Diagnostics.h"
#include "Presets.h"
#include "FollowerOutfits.h"
#include "FollowerAutoOutfits.h"
#include "Transmog.h"
#include <mutex>
#include <set>
#include <map>
#include <random>
#include <nlohmann/json.hpp>

namespace Gallery {
namespace {
using Identity=ManagedIdentity;
// Read existing fields only. Display-name getters can build/cache names, so avoid them.
void LogGeneratedName(const char* stage, RE::TESBoundObject* object, RE::ExtraDataList* extra, RE::FormID actor) {
    if(!(cleanupCompatibility.load() || reclaimRenamedGeneratedItems.load()) || !DetailEnabled() || !object || !extra) return;
    try {
        static unsigned samples{}; // Game-thread only; bounded for this diagnostic session.
        if(samples>=1024) return;
        ++samples;
        const auto* data=extra->GetByType<RE::ExtraTextDisplayData>();
        const auto* uid=extra->GetByType<RE::ExtraUniqueID>();
        const std::string base=object->GetName()?object->GetName():"";
        const std::string stored=data && data->displayName.c_str()?data->displayName.c_str():"";
        nlohmann::json row={{"stage",stage},{"form",object->GetFormID()},{"actor",actor},
            {"uid",uid?uid->uniqueID:0},{"owner",uid?uid->baseID:0},
            {"baseName",base},{"hasTextData",data!=nullptr},{"storedName",stored},
            {"storedMatchesBase",data && stored==base},{"worn",extra->HasType<RE::ExtraWorn>() || extra->HasType<RE::ExtraWornLeft>()}};
        if(data) {
            row["ownerInstance"]=data->ownerInstance.underlying();
            row["customNameLength"]=data->customNameLength;
            row["temperFactor"]=data->temperFactor;
            row["nameMessage"]=data->displayNameText?data->displayNameText->GetFormID():0;
            row["ownerQuest"]=data->ownerQuest?data->ownerQuest->GetFormID():0;
        }
        GALLERY_DIAG("OG-NAME-DIAG {}",row.dump(-1,' ',false,nlohmann::json::error_handler_t::replace));
    } catch(...) {} // Diagnostics never change tracking or deletion decisions.
}

std::mutex ledgerMutex;
ManagedLedger ledger;
using OrdinaryAddition=FreshAdditionObservation;
std::optional<OrdinaryAddition> ordinaryAddition; // only during our AddObject call; guarded by ledgerMutex
std::optional<SmithingCreation> smithingCreation;
std::map<std::pair<RE::FormID,RE::FormID>,int> unverifiedSmithing; // before-count of a failed generation
std::optional<Identity> ownReclaim; // guarded by ledgerMutex; only around our verified RemoveItem call
// Names belong to fresh generation claims, never inferred from current inventory.
std::map<std::pair<RE::FormID,RE::FormID>,RegisteredName> pendingNames;
std::map<Identity,RegisteredName> registeredNames;
std::map<Identity,GeneratedPerformance> generatedPerformance;
bool SimpleName(const RE::ExtraTextDisplayData* n) {
    return n && n->ownerInstance.underlying()==-2 && !n->displayNameText && !n->ownerQuest && n->temperFactor==1.0f;
}
TransferObservation transferObservation;
TransferChain transferChain;
// Shared sequence for raw notifications. Bounded, session-only, no engine access.
unsigned transferEventSequence{};
unsigned NextTransferEventSequence() {
    std::scoped_lock lock(ledgerMutex);
    if(transferEventSequence>=1025) return 0;
    return ++transferEventSequence;
}
class IdentityChanges final : public RE::BSTEventSink<RE::TESUniqueIDChangeEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESUniqueIDChangeEvent* e,RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*) override {
        if(e) {
            const bool changed=e->oldBaseID!=e->newBaseID || e->oldUniqueID!=e->newUniqueID;
            bool creationEvent=false;
            {
                std::scoped_lock lock(ledgerMutex);
                if(smithingCreation && smithingCreation->form==e->objectID) {
                    const bool continuous=e->oldBaseID==smithingCreation->owner && e->oldUniqueID==smithingCreation->current;
                    smithingCreation->Change(e->objectID,e->oldBaseID,e->oldUniqueID,e->newBaseID,e->newUniqueID);
                    creationEvent=continuous && !smithingCreation->invalid;
                    try {SKSE::log::info("OG-SMITHING2 id-event form={:08X} oldOwner={:08X} oldID={} newOwner={:08X} newID={} changed={} invalid={}",
                        e->objectID,e->oldBaseID,e->oldUniqueID,e->newBaseID,e->newUniqueID,changed,smithingCreation->invalid);} catch(...) {}
                }
                // Actual identity changes invalidate performance evidence regardless
                // of diagnostic settings. No-op notifications do not erase claims.
                if(changed && !creationEvent) {
                    generatedPerformance.erase({e->objectID,e->oldBaseID,e->oldUniqueID});
                    generatedPerformance.erase({e->objectID,e->newBaseID,e->newUniqueID});
                }
            }
            if(creationEvent || !changed || !(DetailEnabled() || cleanupCompatibility.load())) return RE::BSEventNotifyControl::kContinue;
            const auto seq=NextTransferEventSequence();
            {
                std::scoped_lock lock(ledgerMutex);
                try {
                    registeredNames.erase({e->objectID,e->oldBaseID,e->oldUniqueID});
                    registeredNames.erase({e->objectID,e->newBaseID,e->newUniqueID});
                    if(seq && seq<=1024) transferChain.Change(ledger,{e->objectID,e->oldBaseID,e->oldUniqueID},{e->objectID,e->newBaseID,e->newUniqueID});
                    else transferChain.Stop();
                } catch(...) {transferChain.Stop();}
            }
            if(seq && seq<=1024) GALLERY_DIAG("OG-TRANSFER-EVENT seq={} type=unique-id object={:08X} oldBase={:08X} newBase={:08X} oldUID={} newUID={} action=observe-only",seq,e->objectID,e->oldBaseID,e->newBaseID,e->oldUniqueID,e->newUniqueID);
            if(seq==1025) GALLERY_DIAG("OG-TRANSFER-EVENT limit=1024 reached; further raw events omitted until diagnostics reset");
        }
        return RE::BSEventNotifyControl::kContinue;
    }
} identityChanges;
// Temporary scalar evidence only; never serialized or kept across an apply/load.
std::map<std::pair<RE::FormID,RE::FormID>,ManagedIdentity> generatedEvidence;
std::map<std::uint16_t,std::uint64_t> preferredScopes;
std::set<std::uint16_t> reservedPreferredIDs;
std::map<std::pair<RE::FormID,std::uint16_t>,std::uint64_t> smithingScopes;
constexpr std::uint32_t preferredRecord=0x454E4348;
constexpr std::uint32_t namespaceID=0x4F474D49; // OGMI
constexpr std::uint32_t itemRecord=0x4954454D; // ITEM

// Include all raw entries, even zero-count entries and IDs originating from
// another container. The engine scans these too. Caller also supplies saved
// reservations and ledger IDs; this function never changes existing identities.
std::uint16_t AllocateItemID(RE::InventoryChanges* changes, std::set<std::uint16_t>& used) {
    if(changes->entryList) for(auto* entry:*changes->entryList) if(entry && entry->extraLists)
        for(auto* extra:*entry->extraLists) if(extra)
            if(auto* uid=extra->GetByType<RE::ExtraUniqueID>()) used.insert(uid->uniqueID);
    const auto candidate=changes->GetNextUniqueID();
    const auto id=FindAvailableItemID(candidate,used);
    return id;
}

void Forget(const Identity& key,bool retain=false) {std::scoped_lock lock(ledgerMutex); ledger.items.erase(key); registeredNames.erase(key); transferChain.Retire(key); if(retain && generatedPerformance.contains(key) && generatedPerformance[key].reproduction) generatedPerformance[key].retained=true; else generatedPerformance.erase(key);}
void Revert(SKSE::SerializationInterface*) {
    ResetTransmogPersistence();
    {std::scoped_lock lock(ledgerMutex); ledger.Clear(); pendingNames.clear(); registeredNames.clear(); generatedPerformance.clear(); smithingCreation.reset(); ordinaryAddition.reset(); unverifiedSmithing.clear(); transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0; generatedEvidence.clear(); preferredScopes.clear(); reservedPreferredIDs.clear(); smithingScopes.clear();}
    ClearFollowerOutfits();
    ClearFollowerAutoOutfits();
}
void Save(SKSE::SerializationInterface* s) {
    SaveTransmog(s);
    SaveFollowerOutfits(s);
    SaveFollowerAutoOutfits(s);
    if(!preferredScopes.empty()) {
        const auto count=static_cast<std::uint32_t>(preferredScopes.size());
        if(!s->OpenRecord(preferredRecord,2) || !s->WriteRecordData(count)) {SKSE::log::error("Enchanted item: identity scope save failed"); return;}
        for(const auto& [id,scope]:preferredScopes) if(!s->WriteRecordData(id) || !s->WriteRecordData(scope)) {SKSE::log::error("Enchanted item: reserved ID save failed"); return;}
    }
    for(const auto& [key,scope]:smithingScopes) {
        if(!s->OpenRecord(0x534D4944,1) || !s->WriteRecordData(key.first) || !s->WriteRecordData(key.second) || !s->WriteRecordData(scope)) {
            SKSE::log::error("Smithing original identity save failed");return;
        }
    }
    std::set<Identity> copy;
    std::map<Identity,RegisteredName> names;
    {std::scoped_lock lock(ledgerMutex); copy=ledger.items; names=registeredNames;}
    for(const auto& k:copy) {
        if(!s->OpenRecord(itemRecord,k.owner==0x14 ? 1 : 2) || !s->WriteRecordData(k.form) || !s->WriteRecordData(k.owner) || !s->WriteRecordData(k.id)) {
            SKSE::log::error("Managed item ledger could not be saved"); return;
        }
    }
    std::map<Identity,GeneratedPerformance> performance;
    {std::scoped_lock lock(ledgerMutex);performance=generatedPerformance;}
    for(const auto& [k,p]:performance) if(copy.contains(k) || (p.reproduction && p.retained)) {
        const std::uint8_t flags=(p.reproduction?1:0)|(p.retained?2:0);
        if(!s->OpenRecord(0x50455246,1) || !s->WriteRecordData(k.form) || !s->WriteRecordData(k.owner) || !s->WriteRecordData(k.id) || !s->WriteRecordData(p.health) || !s->WriteRecordData(flags)) {
            SKSE::log::error("Generated performance evidence save failed");return;
        }
    }
    // Separate optional record: older releases still understand the ordinary ITEM ledger.
    for(const auto& [k,n]:names) if(copy.contains(k) && ValidRegisteredName(n)) {
        const auto a=static_cast<std::uint32_t>(n.base.size()),b=static_cast<std::uint32_t>(n.display.size());
        if(!s->OpenRecord(0x4E414D45,1) || !s->WriteRecordData(k.form) || !s->WriteRecordData(k.owner) || !s->WriteRecordData(k.id) ||
           !s->WriteRecordData(a) || !s->WriteRecordData(n.base.data(),a) || !s->WriteRecordData(b) || !s->WriteRecordData(n.display.data(),b)) {
            SKSE::log::error("Registered name evidence save failed"); return;
        }
    }
}
void Load(SKSE::SerializationInterface* s) {
    ResetTransmogPersistence();
    ClearFollowerOutfits();
    ClearFollowerAutoOutfits();
    preferredScopes.clear(); reservedPreferredIDs.clear(); smithingScopes.clear();
    std::set<Identity> next;
    std::map<Identity,RegisteredName> names;
    std::set<Identity> duplicateNames;
    std::map<Identity,GeneratedPerformance> performance;
    std::set<Identity> duplicatePerformance;
    std::uint32_t type{},version{},length{};
    while(s->GetNextRecordInfo(type,version,length)) {
        if(type==0x534D4944) {
            RE::FormID old{},owner{};std::uint16_t id{};std::uint64_t scope{};
            if(version!=1 || length!=14 || smithingScopes.size()>=10000) continue;
            if(s->ReadRecordData(old)!=4 || s->ReadRecordData(id)!=2 || s->ReadRecordData(scope)!=8 || !id || !scope || !s->ResolveFormID(old,owner) || !owner) continue;
            const auto key=std::make_pair(owner,id);
            const auto [it,inserted]=smithingScopes.emplace(key,scope);if(!inserted) it->second=0; // conflicting scope never authorizes a match
            continue;
        }
        if(type==0x50455246) {
            Identity old{},key{};GeneratedPerformance p;std::uint8_t flags{};
            if(version!=1 || length!=15 || performance.size()>=10000) continue;
            if(s->ReadRecordData(old.form)!=4 || s->ReadRecordData(old.owner)!=4 || s->ReadRecordData(old.id)!=2 || s->ReadRecordData(p.health)!=4 || s->ReadRecordData(flags)!=1) continue;
            if(!old.id || flags>3 || (p.health!=1.f && !ValidSmithing(p.health)) || !s->ResolveFormID(old.form,key.form) || !s->ResolveFormID(old.owner,key.owner) || !key.form || !key.owner) continue;
            key.id=old.id;p.reproduction=flags&1;p.retained=flags&2;
            if((p.retained && !p.reproduction) || (p.reproduction && !ValidSmithing(p.health))) continue;
            if(!performance.emplace(key,p).second) duplicatePerformance.insert(key);
            continue;
        }
        if(type==0x4E414D45) {
            Identity old{},key{}; RegisteredName n; std::uint32_t a{},b{};
            if(version!=1 || length<20 || length>1042 || names.size()>=1024) continue;
            if(s->ReadRecordData(old.form)!=4 || s->ReadRecordData(old.owner)!=4 || s->ReadRecordData(old.id)!=2 || s->ReadRecordData(a)!=4 || !a || a>512 || length<18+a) continue;
            n.base.resize(a);
            if(s->ReadRecordData(n.base.data(),a)!=a || s->ReadRecordData(b)!=4 || !b || b>512 || length!=18+a+b) continue;
            n.display.resize(b);
            if(s->ReadRecordData(n.display.data(),b)!=b || !ValidRegisteredName(n) || !old.id ||
               !s->ResolveFormID(old.form,key.form) || !s->ResolveFormID(old.owner,key.owner) || !key.form || !key.owner) continue;
            key.id=old.id;
            if(!names.emplace(key,std::move(n)).second) duplicateNames.insert(key);
            continue;
        }
        if(type==preferredRecord) {
            std::uint32_t count{};
            if(version!=2 || length<4 || s->ReadRecordData(count)!=4 || count>65535 || length!=4+count*10) continue;
            std::map<std::uint16_t,std::uint64_t> scopes; bool valid=true;
            for(std::uint32_t n=0;n<count;++n) {std::uint16_t id{}; std::uint64_t scope{}; if(s->ReadRecordData(id)!=2 || s->ReadRecordData(scope)!=8 || !id || !scope || !scopes.emplace(id,scope).second) valid=false;}
            if(valid) {preferredScopes=std::move(scopes); for(const auto& [id,scope]:preferredScopes) reservedPreferredIDs.insert(id);}
            continue;
        }
        if(LoadFollowerOutfit(s,type,version,length)) continue;
        if(LoadFollowerAutoOutfit(s,type,version,length)) continue;
        if(LoadTransmog(s,type,version,length)) continue;
        if(type!=itemRecord || (version!=1 && version!=2) || length!=10) continue;
        Identity old, current;
        if(s->ReadRecordData(old.form)!=4 || s->ReadRecordData(old.owner)!=4 || s->ReadRecordData(old.id)!=2) continue;
        if(!old.id || !s->ResolveFormID(old.form,current.form) || !s->ResolveFormID(old.owner,current.owner)) continue;
        current.id=old.id;
        if(current.owner && (version==2 || current.owner==0x14) && next.size()<10000) next.insert(current);
    }
    std::scoped_lock lock(ledgerMutex); ledger.Clear(); pendingNames.clear(); registeredNames.clear(); generatedPerformance.clear(); smithingCreation.reset(); ordinaryAddition.reset(); unverifiedSmithing.clear(); transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0; generatedEvidence.clear(); ledger.items=std::move(next);
    for(auto& [key,n]:names) if(ledger.items.contains(key) && !duplicateNames.contains(key)) registeredNames.emplace(key,std::move(n));
    for(const auto& [key,p]:performance) if(!duplicatePerformance.contains(key) && (ledger.items.contains(key) || (p.reproduction && p.retained))) generatedPerformance.emplace(key,p);
}
class Transfers final : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* e,RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
        if(e) {
            if(DetailEnabled() || cleanupCompatibility.load()) {
                const auto seq=NextTransferEventSequence();
                if(seq && seq<=1024) GALLERY_DIAG("OG-TRANSFER-EVENT seq={} type=container form={:08X} from={:08X} to={:08X} count={} uid={} action=observe-only",seq,e->baseObj,e->oldContainer,e->newContainer,e->itemCount,e->uniqueID);
                if(seq==1025) GALLERY_DIAG("OG-TRANSFER-EVENT limit=1024 reached; further raw events omitted until diagnostics reset");
            }
            // No engine calls or mutations under the event/ledger lock.
            std::size_t released{}, cancelled{}, observed{};
            {
                std::scoped_lock lock(ledgerMutex);
                if(ordinaryAddition && ordinaryAddition->form==e->baseObj) {
                    auto& addition=*ordinaryAddition;
                    // Incoming UID is evidence only for this synchronous addition
                    // from an empty ownership source, never for a manual pickup.
                    const auto pending=ledger.pending.find({addition.actor,addition.form});
                    const auto observed=addition.Observe(e->baseObj,e->oldContainer,e->newContainer,e->itemCount,e->uniqueID,
                        pending!=ledger.pending.end() && pending->second==1);
                    if(observed) generatedEvidence[{addition.actor,addition.form}]={addition.form,addition.actor,observed};
                }
                if(smithingCreation && smithingCreation->form==e->baseObj && smithingCreation->owner==e->oldContainer && e->oldContainer!=e->newContainer) smithingCreation->invalid=true;
                if(ownReclaim && IsOwnReclaimEvent(ownReclaim->form,ownReclaim->owner,ownReclaim->id,
                    e->baseObj,e->oldContainer,e->newContainer,e->itemCount,e->uniqueID)) return RE::BSEventNotifyControl::kContinue;
                if(DetailEnabled() && e->oldContainer && e->oldContainer!=e->newContainer)
                    cancelled=ledger.pending.count({e->oldContainer,e->baseObj});
                if(DetailEnabled() || cleanupCompatibility.load()) {
                    if(transferEventSequence>=1025) transferChain.Stop();
                    else transferChain.Container(e->baseObj,e->oldContainer,e->newContainer,e->itemCount,e->uniqueID);
                    try {observed=transferObservation.Capture(ledger,e->baseObj,e->oldContainer,e->newContainer);}
                    catch(...) {} // diagnostics must not interrupt ordinary transfer handling
                } else {transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0;}
                if(e->oldContainer && e->oldContainer!=e->newContainer) {
                    pendingNames.erase({e->oldContainer,e->baseObj});
                    std::erase_if(registeredNames,[&](const auto& n){return n.first.owner==e->oldContainer && n.first.form==e->baseObj;});
                    std::erase_if(generatedPerformance,[&](const auto& n){return n.first.owner==e->oldContainer && n.first.form==e->baseObj;});
                }
                released=ledger.Transfer(e->baseObj,e->oldContainer,e->newContainer);
                if(!generatedEvidence.empty() && e->oldContainer && e->oldContainer!=e->newContainer) generatedEvidence.erase({e->oldContainer,e->baseObj});
            }
            if(released || cancelled) GALLERY_DIAG("OG-DIAG transfer released={} cancelledPending={} form={:08X} from={:08X} to={:08X} reason=outgoing-transfer",released,cancelled,e->baseObj,e->oldContainer,e->newContainer);
            if(observed) GALLERY_DIAG("OG-TRANSFER-DIAG retained={} form={:08X} from={:08X} to={:08X} action=observe-only",observed,e->baseObj,e->oldContainer,e->newContainer);
        }
        return RE::BSEventNotifyControl::kContinue;
    }
} transfers;
void InspectDepartedItems(RE::Actor* actor) {
    if(!DetailEnabled()) return;
    std::map<Identity,std::uint32_t> copy;
    TransferChain chainCopy;
    {std::scoped_lock lock(ledgerMutex); chainCopy=transferChain; for(const auto& [key,to]:transferObservation.departed)
        if(key.owner==actor->GetFormID()) copy.emplace(key,to);}
    if(copy.empty()) return;
    // Called only from normal game-thread cleanup, never from the event sink.
    const auto inventory=actor->GetInventory();
    for(const auto& [key,to]:copy) {
        int total=0; std::vector<TransferCandidate> candidates;
        for(const auto& [obj,data]:inventory) if(obj && obj->GetFormID()==key.form && data.second) {
            total=data.first;
            if(data.second->extraLists) for(auto* extra:*data.second->extraLists) if(extra) {
                const auto* uid=extra->GetByType<RE::ExtraUniqueID>();
                const bool protectedItem=data.second->IsQuestObject() || extra->HasType<RE::ExtraHealth>() ||
                    extra->HasType<RE::ExtraEnchantment>() || extra->HasType<RE::ExtraTextDisplayData>() ||
                    extra->HasType<RE::ExtraHotkey>() || extra->HasType<RE::ExtraPoison>();
                candidates.push_back({uid?uid->baseID:0,uid?uid->uniqueID:std::uint16_t{},extra->GetCount(),protectedItem,
                    extra->HasType<RE::ExtraWorn>() || extra->HasType<RE::ExtraWornLeft>()});
            }
        }
        GALLERY_DIAG("OG-TRANSFER-DIAG inspect actor={:08X} form={:08X} expectedUID={} destination={:08X} total={} lists={} result={} action=observe-only",key.owner,key.form,key.id,to,total,candidates.size(),ClassifyTransfer(key,total,candidates));
        const auto chainIt=chainCopy.entries.find(key);
        const auto current=chainIt!=chainCopy.entries.end()?chainIt->second.current:Identity{};
        const auto hops=chainIt!=chainCopy.entries.end()?chainIt->second.hops:0;
        GALLERY_DIAG("OG-TRANSFER-CHAIN form={:08X} originalOwner={:08X} originalUID={} currentOwner={:08X} currentUID={} hops={} result={} action=observe-only",key.form,key.owner,key.id,current.owner,current.id,hops,chainCopy.Classify(key,total,candidates));
        unsigned logged=0;
        for(const auto& c:candidates) {
            if(logged++>=32) break;
            GALLERY_DIAG("OG-TRANSFER-DIAG candidate form={:08X} owner={:08X} uid={} count={} protected={} worn={}",key.form,c.owner,c.id,c.count,c.protectedItem,c.worn);
        }
    }
}
}
RegisteredName CaptureRegisteredName(RE::TESBoundObject* object,RE::InventoryEntryData* entry) {
    if(!cleanupCompatibility.load() || !object || !entry || !entry->extraLists) return {};
    RE::ExtraDataList* worn{}; unsigned count{};
    for(auto* extra:*entry->extraLists) if(extra && (extra->HasType<RE::ExtraWorn>() || extra->HasType<RE::ExtraWornLeft>())) {worn=extra;++count;}
    if(count!=1 || worn->GetCount()!=1 || entry->IsQuestObject()) return {};
    const auto* text=worn->GetByType<RE::ExtraTextDisplayData>();
    if(!SimpleName(text) || worn->HasType<RE::ExtraHealth>() || worn->HasType<RE::ExtraEnchantment>() || worn->HasType<RE::ExtraHotkey>() || worn->HasType<RE::ExtraPoison>()) return {};
    RegisteredName result{object->GetName()?object->GetName():"",text->displayName.c_str()?text->displayName.c_str():""};
    return ValidRegisteredName(result)?result:RegisteredName{};
}
void ResetTransferObservations() {std::scoped_lock lock(ledgerMutex);transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0;}
std::uint64_t PreferredScope(std::uint16_t id) {const auto it=preferredScopes.find(id); return it==preferredScopes.end()?0:it->second;}
PreferredItem RememberOwnedInstance(RE::Actor* actor,RE::ExtraDataList* extra) {
    if(!actor || actor->GetFormID()!=0x14 || !extra) return {};
    PreferredItem result; result.custom=true;
    if(extra->GetCount()!=1) return result;
    auto* changes=actor->GetInventoryChanges(); if(!changes) return result;
    auto* uid=extra->GetByType<RE::ExtraUniqueID>();
    // Never replace identities owned by the engine or another mod/container.
    if(uid && (uid->baseID!=0x14 || !uid->uniqueID)) {
        SKSE::log::info("Enchanted item: registration fallback: existing identity owner={:08X} id={} is outside the supported player identity scope",uid->baseID,uid->uniqueID);
        return result;
    }
    if(!uid) {
        std::set<std::uint16_t> used=reservedPreferredIDs;
        for(const auto& [obj,data]:actor->GetInventory()) if(data.second && data.second->extraLists)
            for(auto* e:*data.second->extraLists) if(e) if(auto* id=e->GetByType<RE::ExtraUniqueID>(); id && id->baseID==0x14) used.insert(id->uniqueID);
        {std::scoped_lock lock(ledgerMutex); for(const auto& key:ledger.items) if(key.owner==0x14) used.insert(key.id);}
        const auto id=AllocateItemID(changes,used);
        if(!id) return result;
        uid=new RE::ExtraUniqueID(0x14,id); extra->Add(uid);
    }
    auto& scope=preferredScopes[uid->uniqueID];
    if(!scope) {std::random_device random; do {scope=(static_cast<std::uint64_t>(random())<<32)^random();} while(!scope);}
    reservedPreferredIDs.insert(uid->uniqueID);
    result.scope=scope; result.id=uid->uniqueID;
    result.signature=EnchantmentSignature(extra);
    SKSE::log::debug("Enchanted item: registered player enchanted instance id={}; save the game to persist identity scope",uid->uniqueID);
    return result;
}
void InitializeManagedItems() {
    auto* s=SKSE::GetSerializationInterface();
    s->SetUniqueID(namespaceID); s->SetSaveCallback(Save); s->SetLoadCallback(Load); s->SetRevertCallback(Revert);
}
void RegisterManagedItemEvents() {
    if(auto* source=RE::ScriptEventSourceHolder::GetSingleton()) {source->AddEventSink(&transfers); source->AddEventSink(&identityChanges);}
}
float ReadSmithingQuality(RE::ExtraDataList* extra) {
    const auto* health=extra?extra->GetByType<RE::ExtraHealth>():nullptr;
    return health?health->health:1.f;
}
PreferredItem RememberEnchanted(RE::Actor* actor,RE::ExtraDataList* extra) {
    return extra && extra->HasType<RE::ExtraEnchantment>()?RememberOwnedInstance(actor,extra):PreferredItem{};
}
SmithingRecipe RememberSmithing(RE::Actor* actor,RE::ExtraDataList* extra) {
    const float health=ReadSmithingQuality(extra);
    if(!ValidSmithing(health) || !extra || extra->GetCount()!=1) return {};
    if(!actor) return {health};
    if(actor->GetFormID()==0x14) {
        const auto original=RememberOwnedInstance(actor,extra);
        return {health,original.scope,original.id};
    }
    auto* changes=actor->GetInventoryChanges();if(!changes) return {health};
    auto* uid=extra->GetByType<RE::ExtraUniqueID>();
    if(uid && (uid->baseID!=actor->GetFormID() || !uid->uniqueID)) return {health};
    if(smithingScopes.size()>=10000) return {health};
    if(!uid) {
        std::set<std::uint16_t> used;
        for(const auto& [key,_]:smithingScopes) if(key.first==actor->GetFormID()) used.insert(key.second);
        {std::scoped_lock lock(ledgerMutex);for(const auto& key:ledger.items) if(key.owner==actor->GetFormID()) used.insert(key.id);}
        const auto id=AllocateItemID(changes,used);if(!id) return {health};
        uid=new RE::ExtraUniqueID(actor->GetFormID(),id);extra->Add(uid);
    }
    auto& scope=smithingScopes[{actor->GetFormID(),uid->uniqueID}];
    if(!scope) {std::random_device random;do {scope=(static_cast<std::uint64_t>(random())<<32)^random();} while(!scope);}
    return {health,scope,uid->uniqueID};
}
std::uint16_t FindSmithingInstance(RE::Actor* actor,RE::TESBoundObject* obj,const SmithingRecipe& recipe) {
    if(!actor || !obj || !ValidSmithing(recipe.health)) return 0;
    const auto inventory=actor->GetInventory([obj](RE::TESBoundObject& candidate){return &candidate==obj;});
    const auto found=inventory.find(obj);
    if(found==inventory.end() || found->second.first<=0 || !found->second.second || !found->second.second->extraLists) return 0;
    {std::scoped_lock lock(ledgerMutex);const auto blocked=unverifiedSmithing.find({actor->GetFormID(),obj->GetFormID()});
        if(blocked!=unverifiedSmithing.end() && found->second.first>blocked->second) return 0;}
    const auto followerScope=smithingScopes.find({actor->GetFormID(),recipe.id});
    const auto scope=actor->GetFormID()==0x14?PreferredScope(recipe.id):(followerScope==smithingScopes.end()?0:followerScope->second);
    const bool original=recipe.id && recipe.scope && scope==recipe.scope;
    std::vector<SmithingCandidate> candidates;
    for(auto* extra:*found->second.second->extraLists) if(extra) {
        const auto* uid=extra->GetByType<RE::ExtraUniqueID>();
        if(!uid || uid->baseID!=actor->GetFormID()) continue;
        const Identity key{obj->GetFormID(),uid->baseID,uid->uniqueID};
        bool reproduction=false;
        {std::scoped_lock lock(ledgerMutex);const auto profile=generatedPerformance.find(key);
            reproduction=profile!=generatedPerformance.end() && profile->second.reproduction && profile->second.health==recipe.health;}
        candidates.push_back({uid->uniqueID,extra->GetCount(),original && uid->uniqueID==recipe.id,reproduction});
    }
    const auto selected=SelectSmithingCandidate(candidates);
    GALLERY_DIAG("OG-SMITHING select actor={:08X} form={:08X} uid={} originalScope={} health={}",actor->GetFormID(),obj->GetFormID(),selected,original,recipe.health);
    return selected;
}
std::uint16_t PrepareSmithingInstance(RE::Actor* actor,RE::TESBoundObject* obj,const SmithingRecipe& recipe,bool allowGeneration) {
    if(const auto existing=FindSmithingInstance(actor,obj,recipe)) return existing;
    if(!allowGeneration) throw std::runtime_error("Registered smithing item is missing; enable Add missing base items to reproduce it");
    if(!actor || !obj || !obj->As<RE::TESObjectARMO>() || !ValidSmithing(recipe.health)) throw std::runtime_error("Invalid smithing reproduction request");
    const auto beforeInventory=actor->GetInventory([obj](RE::TESBoundObject& candidate){return &candidate==obj;});
    const auto beforeEntry=beforeInventory.find(obj);
    const int beforeCount=beforeEntry==beforeInventory.end()?0:std::max(0,beforeEntry->second.first);
    std::set<std::pair<std::uint32_t,std::uint16_t>> beforeIDs;
    if(beforeEntry!=beforeInventory.end() && beforeEntry->second.second && beforeEntry->second.second->extraLists)
        for(auto* e:*beforeEntry->second.second->extraLists) if(e) if(const auto* uid=e->GetByType<RE::ExtraUniqueID>()) beforeIDs.emplace(uid->baseID,uid->uniqueID);
    const auto blockedKey=std::make_pair(actor->GetFormID(),obj->GetFormID());
    {std::scoped_lock lock(ledgerMutex);
        const auto blocked=unverifiedSmithing.find(blockedKey);
        if(blocked!=unverifiedSmithing.end()) {
            if(beforeCount>blocked->second) throw std::runtime_error("Generated equipment could not be verified. Further generation is stopped to prevent duplicates. See OutfitGallery.log.");
            unverifiedSmithing.erase(blocked);
        }
        if(smithingCreation) throw std::runtime_error("Nested smithing generation is not supported");
    }
    auto* changes=actor->GetInventoryChanges();if(!changes) throw std::runtime_error("Inventory changes unavailable");
    std::set<std::uint16_t> used=reservedPreferredIDs;
    {std::scoped_lock lock(ledgerMutex);
        if(generatedPerformance.size()>=10000) throw std::runtime_error("Generated performance ledger is full");
        for(const auto& key:ledger.items) if(key.owner==actor->GetFormID()) used.insert(key.id);
        for(const auto& [key,_]:generatedPerformance) if(key.owner==actor->GetFormID()) used.insert(key.id);
    }
    for(const auto& [key,_]:smithingScopes) if(key.first==actor->GetFormID()) used.insert(key.second);
    const auto id=AllocateItemID(changes,used);if(!id) throw std::runtime_error("No item identity available");
    const bool post629=REL::Module::IsAE() && REL::Module::get().version()>=REL::Version(1,6,629,0);
    void* storage=RE::malloc(ExtraListStorageSize(post629));if(!storage) throw std::bad_alloc();
    // Same engine ctor/dtor as alandtse/CommonLibVR ng ExtraDataList.cpp.
    // This project's trimmed CommonLib bundle does not export those wrappers.
    using ListFunction=void(RE::ExtraDataList*);
    static REL::Relocation<ListFunction> construct{RELOCATION_ID(11437,11583)};
    auto destroy=[](RE::ExtraDataList* p){if(p) {
        static REL::Relocation<ListFunction> destruct{RELOCATION_ID(11438,11584)};
        destruct(p);RE::free(p);
    }};
    auto* list=static_cast<RE::ExtraDataList*>(storage);construct(list);
    std::unique_ptr<RE::ExtraDataList,decltype(destroy)> extra(list,destroy);
    extra->Add(new RE::ExtraUniqueID(actor->GetFormID(),id));
    extra->Add(new RE::ExtraHealth(recipe.health));
    const Identity key{obj->GetFormID(),actor->GetFormID(),id};
    // Claim before the engine call. An outgoing transfer can cancel the claim.
    {std::scoped_lock lock(ledgerMutex);ledger.items.insert(key);generatedPerformance[key]={recipe.health,true,false};smithingCreation=SmithingCreation{key.form,key.owner,id,id};}
    struct CreationScope {~CreationScope(){std::scoped_lock lock(ledgerMutex);smithingCreation.reset();}} creationScope;
    SKSE::log::info("OG-SMITHING2 begin actor={:08X} form={:08X} allocatedID={} before={} health={}",key.owner,key.form,id,beforeCount,recipe.health);
    actor->AddObjectToContainer(obj,extra.release(),1,nullptr); // engine takes ownership of the runtime-sized list
    SmithingCreation proof;
    {std::scoped_lock lock(ledgerMutex);if(smithingCreation) proof=*smithingCreation;else proof.invalid=true;}
    // Keep inspection separate from identity repair. The returned pointer is
    // used only before the event broadcast, and is reacquired afterward.
    auto inspect=[&](const SmithingCreation& current,const char* stage) {
        const auto afterInventory=actor->GetInventory([obj](RE::TESBoundObject& candidate){return &candidate==obj;});
        const auto afterEntry=afterInventory.find(obj);
        SmithingGenerationEvidence evidence;
        evidence.invalid=current.invalid;evidence.before=beforeCount;evidence.after=afterEntry==afterInventory.end()?0:std::max(0,afterEntry->second.first);
        evidence.actor=key.owner;evidence.expected=current.current;evidence.requested=recipe.health;
        RE::ExtraDataList* candidate{};
        if(afterEntry!=afterInventory.end() && afterEntry->second.second && afterEntry->second.second->extraLists) {
            unsigned logged{};
            for(auto* e:*afterEntry->second.second->extraLists) if(e) {
                const auto* uid=e->GetByType<RE::ExtraUniqueID>();
                if(logged++<32) SKSE::log::info("OG-SMITHING3 candidate stage={} form={:08X} owner={:08X} uid={} count={} health={} enchanted={} existed={}",
                    stage,key.form,uid?uid->baseID:0,uid?uid->uniqueID:0,e->GetCount(),ReadSmithingQuality(e),e->HasType<RE::ExtraEnchantment>(),uid && beforeIDs.contains({uid->baseID,uid->uniqueID}));
                if(!uid || uid->baseID!=key.owner || uid->uniqueID!=current.current) continue;
                candidate=e;++evidence.matches;evidence.owner=uid->baseID;evidence.actual=uid->uniqueID;evidence.instanceCount=e->GetCount();
                evidence.actualHealth=ReadSmithingQuality(e);evidence.customEnchanted=e->HasType<RE::ExtraEnchantment>();
                evidence.wasPresent=beforeIDs.contains({uid->baseID,uid->uniqueID});evidence.reserved=used.contains(uid->uniqueID);
            }
        }
        return std::pair{evidence,candidate};
    };
    auto [evidence,candidate]=inspect(proof,"after-add");
    bool allocatedOccupied=false;
    // Include raw zero-count lists, as in the initial allocator. Repair must
    // not reuse an ID claimed by an item added reentrantly by another system.
    if(changes->entryList) for(auto* entry:*changes->entryList) if(entry && entry->extraLists)
        for(auto* e:*entry->extraLists) if(e && e!=candidate) if(const auto* uid=e->GetByType<RE::ExtraUniqueID>())
            if(uid->baseID==key.owner && uid->uniqueID==id) allocatedOccupied=true;
    auto* events=RE::ScriptEventSourceHolder::GetSingleton();
    if(candidate && events && CanRetagSmithingGeneration(evidence,id,used.contains(id),allocatedOccupied)) {
        auto* uid=candidate->GetByType<RE::ExtraUniqueID>();
        const auto previous=uid->uniqueID;
        // Only the newly created singleton proved above is edited. Original
        // possessions and original-instance scope reservations stay unchanged.
        uid->uniqueID=id;
        SKSE::log::info("OG-SMITHING3 retag form={:08X} owner={:08X} engineID={} allocatedID={} reason=engine-used-reserved-id",key.form,key.owner,previous,id);
        const RE::TESUniqueIDChangeEvent notification{key.owner,key.owner,key.form,previous,id};
        events->SendEvent(&notification);
        candidate=nullptr; // event callbacks can rebuild lists; never reuse it
        {std::scoped_lock lock(ledgerMutex);if(smithingCreation) proof=*smithingCreation;else proof.invalid=true;}
        evidence=inspect(proof,"after-retag").first;
    }
    const bool verified=VerifiedSmithingGeneration(evidence);
    SKSE::log::info("OG-SMITHING2 result form={:08X} allocatedID={} currentID={} hops={} invalid={} before={} after={} matches={} count={} requested={} actual={} preexisting={} reserved={} verified={}",
        key.form,id,proof.current,proof.hops,proof.invalid,evidence.before,evidence.after,evidence.matches,evidence.instanceCount,recipe.health,evidence.actualHealth,evidence.wasPresent,evidence.reserved,verified);
    if(!verified) {
        // Do not guess an identity, change an existing item or generate repeatedly.
        {std::scoped_lock lock(ledgerMutex);unverifiedSmithing[blockedKey]=beforeCount;
            ledger.items.erase(key);generatedPerformance.erase(key);transferChain.Retire(key);
            if(proof.current!=id && !used.contains(proof.current)) {
                const Identity unverified{key.form,key.owner,proof.current};ledger.items.erase(unverified);generatedPerformance.erase(unverified);transferChain.Retire(unverified);
            }
        }
        throw std::runtime_error("Generated equipment could not be verified. Further generation is stopped to prevent duplicates. See OutfitGallery.log.");
    }
    const Identity actual{key.form,key.owner,proof.current};
    {std::scoped_lock lock(ledgerMutex);
        if(actual!=key) {ledger.items.erase(key);generatedPerformance.erase(key);transferChain.Retire(key);}
        transferChain.Retire(actual);
        ledger.items.insert(actual);generatedPerformance[actual]={recipe.health,true,false};unverifiedSmithing.erase(blockedKey);
    }
    SKSE::log::info("OG-SMITHING generated actor={:08X} form={:08X} uid={} health={}",actual.owner,actual.form,actual.id,recipe.health);
    return actual.id;
}
void BeginManagedAddition(RE::Actor* actor) {
    if(actor) {std::scoped_lock lock(ledgerMutex); ledger.Begin(actor->GetFormID()); std::erase_if(pendingNames,[&](const auto& n){return n.first.first==actor->GetFormID();}); std::erase_if(generatedEvidence,[&](const auto& e){return e.first.first==actor->GetFormID();});}
}
void AddManagedItems(RE::Actor* actor,RE::TESBoundObject* obj,int count,bool previouslyAbsent,const RegisteredName& name) {
    if(!actor || !obj || count<=0) return;
    // Record before AddObject: a synchronous outgoing transfer must be able to
    // cancel the claim. Incoming add events do not claim existing possessions.
    if(previouslyAbsent) {std::scoped_lock lock(ledgerMutex); ledger.pending[{actor->GetFormID(),obj->GetFormID()}]=count;
        if(cleanupCompatibility.load() && count==1 && ValidRegisteredName(name) && pendingNames.size()<1024)
            pendingNames[{actor->GetFormID(),obj->GetFormID()}]=name;}
    GALLERY_DIAG("OG-DIAG generate actor={:08X} form={:08X} count={} previouslyAbsent={}",actor->GetFormID(),obj->GetFormID(),count,previouslyAbsent);
    const bool observe=previouslyAbsent && count==1;
    struct AdditionScope {
        bool active{};
        ~AdditionScope(){if(active) {std::scoped_lock lock(ledgerMutex);ordinaryAddition.reset();}}
    } additionScope;
    if(observe) {
        std::scoped_lock lock(ledgerMutex);
        if(ordinaryAddition) throw std::runtime_error("Nested ordinary item addition is not supported");
        ordinaryAddition=OrdinaryAddition{actor->GetFormID(),obj->GetFormID(),false};additionScope.active=true;
    }
    actor->AddObjectToContainer(obj,nullptr,count,nullptr);
}
void ObserveGeneratedItems(RE::Actor* actor) {
    CleanupTiming timing{"observe"};
    if(!actor) return;
    const auto owner=actor->GetFormID();
    std::map<RE::FormID,int> pending;
    {std::scoped_lock lock(ledgerMutex); for(const auto& [key,count]:ledger.pending) if(key.first==owner) pending[key.second]=count;}
    if(pending.empty()) return; // No inventory query for ordinary owned-item switches.
    timing.items=static_cast<unsigned>(pending.size()); timing.inventoryScan=true;
    const auto inventory=actor->GetInventory();
    for(const auto& [obj,data]:inventory) {
        if(!obj) continue;
        const auto it=pending.find(obj->GetFormID());
        if(it==pending.end() || it->second!=1 || data.first!=1 || !data.second || !data.second->extraLists) continue;
        RE::ExtraDataList* only{}; unsigned lists=0;
        for(auto* extra:*data.second->extraLists) if(extra) {only=extra; ++lists;}
        if(lists!=1 || !only || only->GetCount()!=1) continue;
        LogGeneratedName("after-generation",obj,only,owner);
        auto* uid=only->GetByType<RE::ExtraUniqueID>();
        if(!uid || uid->baseID!=owner || !uid->uniqueID) continue;
        std::scoped_lock lock(ledgerMutex);
        if(ledger.pending.contains({owner,obj->GetFormID()})) {
            generatedEvidence[{owner,obj->GetFormID()}]={obj->GetFormID(),owner,uid->uniqueID};
            GALLERY_DIAG("OG-DIAG compatibility observed form={:08X} uid={}",obj->GetFormID(),uid->uniqueID);
        }
    }
}
void TrackNewManagedItems(RE::Actor* actor, bool compatibility) {
    CleanupTiming timing{"track"};
    if(!actor) return;
    std::map<RE::FormID,int> added;
    std::map<RE::FormID,RegisteredName> names;
    std::map<RE::FormID,ManagedIdentity> evidence;
    std::set<std::uint16_t> ledgerIDs;
    {
        std::scoped_lock lock(ledgerMutex); added=ledger.TakePending(actor->GetFormID());
        if(compatibility) for(const auto& [k,n]:pendingNames) if(k.first==actor->GetFormID() && added.contains(k.second)) names[k.second]=n;
        std::erase_if(pendingNames,[&](const auto& n){return n.first.first==actor->GetFormID();});
        for(const auto& [key,value]:generatedEvidence) if(key.first==actor->GetFormID()) evidence[key.second]=value;
        std::erase_if(generatedEvidence,[&](const auto& e){return e.first.first==actor->GetFormID();});
        if(!added.empty()) {
            for(const auto& key:ledger.items) if(key.owner==actor->GetFormID()) ledgerIDs.insert(key.id);
            for(const auto& [key,_]:generatedPerformance) if(key.owner==actor->GetFormID()) ledgerIDs.insert(key.id);
        }
    }
    if(added.empty()) return;
    timing.items=static_cast<unsigned>(added.size());
    auto* changes=actor->GetInventoryChanges(); if(!changes) return;
    timing.inventoryScan=true;
    auto inventory=actor->GetInventory();
    GALLERY_DIAG("OG-DIAG track actor={:08X} pending={} compatibility={}",actor->GetFormID(),added.size(),compatibility);
    if(DetailEnabled()) for(const auto& [form,count]:added) {
        const bool present=std::any_of(inventory.begin(),inventory.end(),[&](const auto& entry){return entry.first && entry.first->GetFormID()==form;});
        if(!present) GALLERY_DIAG("OG-DIAG track skip form={:08X} expected={} reason=not-in-inventory",form,count);
    }
    std::set<std::uint16_t> used;
    std::map<std::uint16_t,unsigned> idOccurrences;
    if(actor->GetFormID()==0x14) used=reservedPreferredIDs;
    for(const auto& [key,_]:smithingScopes) if(key.first==actor->GetFormID()) {used.insert(key.second);ledgerIDs.insert(key.second);}
    for(const auto& [_,data]:inventory) if(data.second && data.second->extraLists) {
        for(auto* extra:*data.second->extraLists) if(extra) if(auto* id=extra->GetByType<RE::ExtraUniqueID>()) {
            if(id->baseID==actor->GetFormID()) {used.insert(id->uniqueID); if(!evidence.empty()) ++idOccurrences[id->uniqueID];}
        }
    }
    {std::scoped_lock lock(ledgerMutex); for(const auto& k:ledger.items) if(k.owner==actor->GetFormID()) used.insert(k.id);}
    for(const auto& [obj,data]:inventory) {
        if(!obj) continue;
        const auto found=added.find(obj->GetFormID());
        if(found==added.end()) continue;
        if(data.first!=found->second || !data.second || !data.second->extraLists) {
            GALLERY_DIAG("OG-DIAG track skip form={:08X} expected={} count={} reason=count-or-extra-list",obj->GetFormID(),found->second,data.first);
            continue;
        }
        std::vector<RE::ExtraDataList*> instances;
        for(auto* extra:*data.second->extraLists) if(extra && extra->GetCount()==1 && (extra->HasType<RE::ExtraWorn>() || extra->HasType<RE::ExtraWornLeft>())) instances.push_back(extra);
        if(instances.size()!=static_cast<std::size_t>(found->second)) {
            GALLERY_DIAG("OG-DIAG track skip form={:08X} expected={} wornSingletons={} reason=worn-instance-count",obj->GetFormID(),found->second,instances.size());
            continue;
        }
        for(auto* extra:instances) {
            LogGeneratedName("after-equip",obj,extra,actor->GetFormID());
            // Use an engine-created worn list: its allocation/layout differs on
            // pre/post 1.6.629. Never allocate ExtraDataList using compile-time size.
            auto* uid=extra->GetByType<RE::ExtraUniqueID>();
            if(!uid && evidence.contains(obj->GetFormID())) {
                GALLERY_DIAG("OG-DIAG compatibility skip form={:08X} reason=observed-id-lost",obj->GetFormID());
                continue;
            }
            if(uid) {
                const auto proof=evidence.find(obj->GetFormID());
                const bool reserved=ledgerIDs.contains(uid->uniqueID) || (actor->GetFormID()==0x14 && reservedPreferredIDs.contains(uid->uniqueID));
                const GeneratedIdentityEvidence check{true,proof!=evidence.end(),actor->GetFormID(),
                    proof==evidence.end()?0:proof->second.owner,uid->baseID,
                    proof==evidence.end()?std::uint16_t{}:proof->second.id,uid->uniqueID,
                    found->second,data.first,extra->GetCount(),idOccurrences[uid->uniqueID],reserved};
                // The strict identity proof is independent of compatibility mode.
                // Other pre-numbered possessions never enter this fresh-add path.
                unsigned formLists{};for(auto* list:*data.second->extraLists) if(list) ++formLists;
                if(!CanAdoptGeneratedIdentity(check,formLists)) {
                    GALLERY_DIAG("OG-DIAG compatibility skip form={:08X} existingID={} observed={} owner={:08X} matches={} reserved={}",obj->GetFormID(),uid->uniqueID,proof!=evidence.end(),uid->baseID,check.matchingIDs,reserved);
                    continue;
                }
                {std::scoped_lock lock(ledgerMutex); ledger.items.insert({obj->GetFormID(),actor->GetFormID(),uid->uniqueID});generatedPerformance[{obj->GetFormID(),actor->GetFormID(),uid->uniqueID}]={};
                if(compatibility && found->second==1 && names.contains(obj->GetFormID()) && registeredNames.size()<1024)
                    registeredNames[{obj->GetFormID(),actor->GetFormID(),uid->uniqueID}]=names.at(obj->GetFormID());}
                ledgerIDs.insert(uid->uniqueID);
                SKSE::log::info("OG-GENERATED4 adopted fresh item form={:08X} owner={:08X} uid={} count=1 compatibility={} (existing ID unchanged)",obj->GetFormID(),actor->GetFormID(),uid->uniqueID,compatibility);
                continue;
            }
            const auto id=AllocateItemID(changes,used);
            if(!id) continue;
            extra->Add(new RE::ExtraUniqueID(actor->GetFormID(),id));
            used.insert(id);
            {std::scoped_lock lock(ledgerMutex); ledger.items.insert({obj->GetFormID(),actor->GetFormID(),id});generatedPerformance[{obj->GetFormID(),actor->GetFormID(),id}]={};
                if(compatibility && found->second==1 && names.contains(obj->GetFormID()) && registeredNames.size()<1024)
                    registeredNames[{obj->GetFormID(),actor->GetFormID(),id}]=names.at(obj->GetFormID());}
            SKSE::log::info("Tracking gallery item {:08X}, instance {}, actor {:08X}",obj->GetFormID(),id,actor->GetFormID());
        }
    }
}
void ReclaimManagedItems(RE::Actor* actor) {
    CleanupTiming timing{"reclaim"};
    if(!actor || !actor->Get3D() || actor->IsDead() || actor->IsDisabled()) return;
    if(actor!=RE::PlayerCharacter::GetSingleton() && (!IsCurrentOutfitFollower(actor) || actor->IsInCombat())) return;
    try {InspectDepartedItems(actor);} catch(...) {GALLERY_DIAG("OG-TRANSFER-DIAG inspection failed; ordinary cleanup unchanged");}
    std::set<Identity> copy;
    {
        std::scoped_lock lock(ledgerMutex); copy=ledger.ForActor(actor->GetFormID());
        if(cleanupCompatibility.load()) for(const auto& [root,e]:transferChain.entries)
            if(e.current.owner==actor->GetFormID() && transferChain.Ready(e.current,true)) copy.insert(e.current);
    }
    timing.items=static_cast<unsigned>(copy.size()); timing.inventoryScan=!copy.empty();
    GALLERY_DIAG("OG-DIAG reclaim actor={:08X} tracked={}",actor->GetFormID(),copy.size());
    for(const auto& key:copy) {
        // Re-read after each removal: removing an item can invalidate extra lists.
        auto inventory=actor->GetInventory();
        RE::TESBoundObject* object{};
        RE::ExtraDataList* instance{};
        unsigned matches{}; bool protectedItem=false; int beforeCount{};
        for(const auto& [obj,data]:inventory) if(obj && obj->GetFormID()==key.form && data.second && data.second->extraLists) {
            protectedItem=data.second->IsQuestObject();
            beforeCount=data.first;
            for(auto* extra:*data.second->extraLists) if(extra) if(auto* uid=extra->GetByType<RE::ExtraUniqueID>()) {
                if(uid->baseID==key.owner && uid->uniqueID==key.id) {object=obj; instance=extra; ++matches;}
            }
        }
        if(matches!=1 || !instance || instance->GetCount()!=1 || beforeCount<=0 || !object || !object->As<RE::TESObjectARMO>()) {
            SKSE::log::info("Preserving ambiguous gallery item {:08X}, instance {}, matches={}, count={}",key.form,key.id,matches,instance?instance->GetCount():0);
            Forget(key); continue;
        }
        LogGeneratedName("before-reclaim",object,instance,actor->GetFormID());
        GeneratedPerformance baseline;bool knownPerformance=false;
        {std::scoped_lock lock(ledgerMutex);const auto it=generatedPerformance.find(key);if(it!=generatedPerformance.end() && !it->second.retained) {baseline=it->second;knownPerformance=true;}}
        const bool ignoreName=knownPerformance || reclaimRenamedGeneratedItems.load();
        const bool hasName=instance->HasType<RE::ExtraTextDisplayData>();
        if(ignoreName && hasName) GALLERY_DIAG("OG-NAME-COMPAT bypass form={:08X} uid={} reason=name-only-protection-disabled",key.form,key.id);
        // Compatibility ignores text changes only. Identity and ownership are
        // still checked above and claimed below; all other protections remain.
        if(knownPerformance) GALLERY_DIAG("OG-PERFORMANCE reclaim actor={:08X} form={:08X} uid={} initial={} current={} reproduction={} customEnchant={} named={}",
            key.owner,key.form,key.id,baseline.health,ReadSmithingQuality(instance),baseline.reproduction,instance->HasType<RE::ExtraEnchantment>(),hasName);
        if(ProtectGeneratedItem(ignoreName,protectedItem,
            knownPerformance?ProtectPerformance(baseline.health,ReadSmithingQuality(instance),false):instance->HasType<RE::ExtraHealth>(),instance->HasType<RE::ExtraEnchantment>(),
            hasName,instance->HasType<RE::ExtraHotkey>(),instance->HasType<RE::ExtraPoison>())) {
            SKSE::log::info("Preserving customized gallery item {:08X}, instance {}: quest={}, health={}, enchant={}, name={}, favorite={}, poison={}",key.form,key.id,protectedItem,instance->HasType<RE::ExtraHealth>(),instance->HasType<RE::ExtraEnchantment>(),instance->HasType<RE::ExtraTextDisplayData>(),instance->HasType<RE::ExtraHotkey>(),instance->HasType<RE::ExtraPoison>());
            Forget(key,knownPerformance && baseline.reproduction); continue;
        }
        if(instance->HasType<RE::ExtraWorn>() || instance->HasType<RE::ExtraWornLeft>()) {
            GALLERY_DIAG("OG-DIAG reclaim skip form={:08X} uid={} reason=still-worn",key.form,key.id); continue;
        }
        bool claimed;
        bool transferClaim=false;
        {
            std::scoped_lock lock(ledgerMutex);
            claimed=ledger.items.erase(key)!=0; registeredNames.erase(key); generatedPerformance.erase(key);
            if(claimed) transferChain.Retire(key);
            else {transferClaim=transferChain.Consume(key,cleanupCompatibility.load()); claimed=transferClaim;}
        }
        if(transferClaim) GALLERY_DIAG("OG-TRANSFER-RECLAIM form={:08X} owner={:08X} uid={} reason=confirmed-roundtrip",key.form,key.owner,key.id);
        if(!claimed) {GALLERY_DIAG("OG-DIAG reclaim skip form={:08X} uid={} reason=claim-released",key.form,key.id); continue;}
        // Explicit instance, count one. Never a base-form-only removal fallback.
        GALLERY_DIAG("OG-DIAG remove actor={:08X} form={:08X} uid={} beforeCount={}",key.owner,key.form,key.id,beforeCount);
        {
            struct OwnReclaimScope {
                explicit OwnReclaimScope(const Identity& identity) {std::scoped_lock lock(ledgerMutex);ownReclaim=identity;}
                ~OwnReclaimScope() {std::scoped_lock lock(ledgerMutex);ownReclaim.reset();}
            } reclaimScope(key);
            actor->RemoveItem(object,1,RE::ITEM_REMOVE_REASON::kRemove,instance,nullptr);
        }
        const auto after=actor->GetInventory();
        const auto found=after.find(object);
        const auto afterCount=found==after.end()?0:found->second.first;
        bool identityRemains=false;
        if(found!=after.end() && found->second.second && found->second.second->extraLists) {
            for(auto* extra:*found->second.second->extraLists) if(extra) if(auto* uid=extra->GetByType<RE::ExtraUniqueID>()) {
                if(uid->baseID==key.owner && uid->uniqueID==key.id) identityRemains=true;
            }
        }
        if(afterCount==beforeCount-1 && !identityRemains) SKSE::log::info("Reclaimed gallery item {:08X}, instance {}, actor {:08X}, inventory {} -> {} (verified)",key.form,key.id,key.owner,beforeCount,afterCount);
        else SKSE::log::warn("Gallery removal not confirmed {:08X}, instance {}, inventory {} -> {}, identity remains={}; no base-form fallback",key.form,key.id,beforeCount,afterCount,identityRemains);
    }
}
}
