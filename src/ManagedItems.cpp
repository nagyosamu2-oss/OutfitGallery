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
#include <mutex>
#include <set>
#include <map>
#include <random>

namespace Gallery {
namespace {
using Identity=ManagedIdentity;
std::mutex ledgerMutex;
ManagedLedger ledger;
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
        if(e && (DetailEnabled() || cleanupCompatibility.load())) {
            const auto seq=NextTransferEventSequence();
            {
                std::scoped_lock lock(ledgerMutex);
                try {
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

void Forget(const Identity& key) {std::scoped_lock lock(ledgerMutex); ledger.items.erase(key); transferChain.Retire(key);}
void Revert(SKSE::SerializationInterface*) {
    {std::scoped_lock lock(ledgerMutex); ledger.Clear(); transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0; generatedEvidence.clear(); preferredScopes.clear(); reservedPreferredIDs.clear();}
    ClearFollowerOutfits();
}
void Save(SKSE::SerializationInterface* s) {
    SaveFollowerOutfits(s);
    if(!preferredScopes.empty()) {
        const auto count=static_cast<std::uint32_t>(preferredScopes.size());
        if(!s->OpenRecord(preferredRecord,2) || !s->WriteRecordData(count)) {SKSE::log::error("Enchanted item: identity scope save failed"); return;}
        for(const auto& [id,scope]:preferredScopes) if(!s->WriteRecordData(id) || !s->WriteRecordData(scope)) {SKSE::log::error("Enchanted item: reserved ID save failed"); return;}
    }
    std::set<Identity> copy;
    {std::scoped_lock lock(ledgerMutex); copy=ledger.items;}
    for(const auto& k:copy) {
        if(!s->OpenRecord(itemRecord,k.owner==0x14 ? 1 : 2) || !s->WriteRecordData(k.form) || !s->WriteRecordData(k.owner) || !s->WriteRecordData(k.id)) {
            SKSE::log::error("Managed item ledger could not be saved"); return;
        }
    }
}
void Load(SKSE::SerializationInterface* s) {
    ClearFollowerOutfits();
    preferredScopes.clear(); reservedPreferredIDs.clear();
    std::set<Identity> next;
    std::uint32_t type{},version{},length{};
    while(s->GetNextRecordInfo(type,version,length)) {
        if(type==preferredRecord) {
            std::uint32_t count{};
            if(version!=2 || length<4 || s->ReadRecordData(count)!=4 || count>65535 || length!=4+count*10) continue;
            std::map<std::uint16_t,std::uint64_t> scopes; bool valid=true;
            for(std::uint32_t n=0;n<count;++n) {std::uint16_t id{}; std::uint64_t scope{}; if(s->ReadRecordData(id)!=2 || s->ReadRecordData(scope)!=8 || !id || !scope || !scopes.emplace(id,scope).second) valid=false;}
            if(valid) {preferredScopes=std::move(scopes); for(const auto& [id,scope]:preferredScopes) reservedPreferredIDs.insert(id);}
            continue;
        }
        if(LoadFollowerOutfit(s,type,version,length)) continue;
        if(type!=itemRecord || (version!=1 && version!=2) || length!=10) continue;
        Identity old, current;
        if(s->ReadRecordData(old.form)!=4 || s->ReadRecordData(old.owner)!=4 || s->ReadRecordData(old.id)!=2) continue;
        if(!old.id || !s->ResolveFormID(old.form,current.form) || !s->ResolveFormID(old.owner,current.owner)) continue;
        current.id=old.id;
        if(current.owner && (version==2 || current.owner==0x14) && next.size()<10000) next.insert(current);
    }
    std::scoped_lock lock(ledgerMutex); ledger.Clear(); transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0; generatedEvidence.clear(); ledger.items=std::move(next);
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
                if(DetailEnabled() && e->oldContainer && e->oldContainer!=e->newContainer)
                    cancelled=ledger.pending.count({e->oldContainer,e->baseObj});
                if(DetailEnabled() || cleanupCompatibility.load()) {
                    if(transferEventSequence>=1025) transferChain.Stop();
                    else transferChain.Container(e->baseObj,e->oldContainer,e->newContainer,e->itemCount,e->uniqueID);
                    try {observed=transferObservation.Capture(ledger,e->baseObj,e->oldContainer,e->newContainer);}
                    catch(...) {} // diagnostics must not interrupt ordinary transfer handling
                } else {transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0;}
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
void ResetTransferObservations() {std::scoped_lock lock(ledgerMutex);transferObservation.Clear(); transferChain.Clear(); transferEventSequence=0;}
std::uint64_t PreferredScope(std::uint16_t id) {const auto it=preferredScopes.find(id); return it==preferredScopes.end()?0:it->second;}
PreferredItem RememberEnchanted(RE::Actor* actor,RE::ExtraDataList* extra) {
    if(!actor || actor->GetFormID()!=0x14 || !extra || !extra->HasType<RE::ExtraEnchantment>()) return {};
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
void BeginManagedAddition(RE::Actor* actor) {
    if(actor) {std::scoped_lock lock(ledgerMutex); ledger.Begin(actor->GetFormID()); std::erase_if(generatedEvidence,[&](const auto& e){return e.first.first==actor->GetFormID();});}
}
void AddManagedItems(RE::Actor* actor,RE::TESBoundObject* obj,int count,bool previouslyAbsent) {
    if(!actor || !obj || count<=0) return;
    // Record before AddObject: a synchronous outgoing transfer must be able to
    // cancel the claim. Incoming add events do not claim existing possessions.
    if(previouslyAbsent) {std::scoped_lock lock(ledgerMutex); ledger.pending[{actor->GetFormID(),obj->GetFormID()}]=count;}
    GALLERY_DIAG("OG-DIAG generate actor={:08X} form={:08X} count={} previouslyAbsent={}",actor->GetFormID(),obj->GetFormID(),count,previouslyAbsent);
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
    std::map<RE::FormID,ManagedIdentity> evidence;
    std::set<std::uint16_t> ledgerIDs;
    {
        std::scoped_lock lock(ledgerMutex); added=ledger.TakePending(actor->GetFormID());
        if(compatibility) for(const auto& [key,value]:generatedEvidence) if(key.first==actor->GetFormID()) evidence[key.second]=value;
        std::erase_if(generatedEvidence,[&](const auto& e){return e.first.first==actor->GetFormID();});
        if(compatibility && !added.empty()) for(const auto& key:ledger.items) if(key.owner==actor->GetFormID()) ledgerIDs.insert(key.id);
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
            // Use an engine-created worn list: its allocation/layout differs on
            // pre/post 1.6.629. Never allocate ExtraDataList using compile-time size.
            auto* uid=extra->GetByType<RE::ExtraUniqueID>();
            if(!uid && evidence.contains(obj->GetFormID())) {
                GALLERY_DIAG("OG-DIAG compatibility skip form={:08X} reason=observed-id-lost",obj->GetFormID());
                continue;
            }
            if(uid) {
                if(!compatibility) {
                    GALLERY_DIAG("OG-DIAG track skip form={:08X} owner={:08X} uid={} reason=existing-id-compatibility-off",obj->GetFormID(),uid->baseID,uid->uniqueID);
                    continue;
                }
                const auto proof=evidence.find(obj->GetFormID());
                const bool reserved=ledgerIDs.contains(uid->uniqueID) || (actor->GetFormID()==0x14 && reservedPreferredIDs.contains(uid->uniqueID));
                const GeneratedIdentityEvidence check{true,proof!=evidence.end(),actor->GetFormID(),
                    proof==evidence.end()?0:proof->second.owner,uid->baseID,
                    proof==evidence.end()?std::uint16_t{}:proof->second.id,uid->uniqueID,
                    found->second,data.first,extra->GetCount(),idOccurrences[uid->uniqueID],reserved};
                if(!CanAdoptGeneratedIdentity(check)) {
                    GALLERY_DIAG("OG-DIAG compatibility skip form={:08X} existingID={} observed={} owner={:08X} matches={} reserved={}",obj->GetFormID(),uid->uniqueID,proof!=evidence.end(),uid->baseID,check.matchingIDs,reserved);
                    continue;
                }
                {std::scoped_lock lock(ledgerMutex); ledger.items.insert({obj->GetFormID(),actor->GetFormID(),uid->uniqueID});}
                ledgerIDs.insert(uid->uniqueID);
                GALLERY_DIAG("OG-DIAG compatibility adopted form={:08X} uid={} (existing ID unchanged)",obj->GetFormID(),uid->uniqueID);
                continue;
            }
            const auto id=AllocateItemID(changes,used);
            if(!id) continue;
            extra->Add(new RE::ExtraUniqueID(actor->GetFormID(),id));
            used.insert(id);
            {std::scoped_lock lock(ledgerMutex); ledger.items.insert({obj->GetFormID(),actor->GetFormID(),id});}
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
        // User customization relinquishes our claim. Never remove tempered,
        // enchanted, renamed, favorited or quest equipment automatically.
        if(protectedItem || instance->HasType<RE::ExtraHealth>() || instance->HasType<RE::ExtraEnchantment>() || instance->HasType<RE::ExtraTextDisplayData>() || instance->HasType<RE::ExtraHotkey>() || instance->HasType<RE::ExtraPoison>()) {
            SKSE::log::info("Preserving customized gallery item {:08X}, instance {}: quest={}, health={}, enchant={}, name={}, favorite={}, poison={}",key.form,key.id,protectedItem,instance->HasType<RE::ExtraHealth>(),instance->HasType<RE::ExtraEnchantment>(),instance->HasType<RE::ExtraTextDisplayData>(),instance->HasType<RE::ExtraHotkey>(),instance->HasType<RE::ExtraPoison>());
            Forget(key); continue;
        }
        if(instance->HasType<RE::ExtraWorn>() || instance->HasType<RE::ExtraWornLeft>()) {
            GALLERY_DIAG("OG-DIAG reclaim skip form={:08X} uid={} reason=still-worn",key.form,key.id); continue;
        }
        bool claimed;
        bool transferClaim=false;
        {
            std::scoped_lock lock(ledgerMutex);
            claimed=ledger.items.erase(key)!=0;
            if(claimed) transferChain.Retire(key);
            else {transferClaim=transferChain.Consume(key,cleanupCompatibility.load()); claimed=transferClaim;}
        }
        if(transferClaim) GALLERY_DIAG("OG-TRANSFER-RECLAIM form={:08X} owner={:08X} uid={} reason=confirmed-roundtrip",key.form,key.owner,key.id);
        if(!claimed) {GALLERY_DIAG("OG-DIAG reclaim skip form={:08X} uid={} reason=claim-released",key.form,key.id); continue;}
        // Explicit instance, count one. Never a base-form-only removal fallback.
        GALLERY_DIAG("OG-DIAG remove actor={:08X} form={:08X} uid={} beforeCount={}",key.owner,key.form,key.id,beforeCount);
        actor->RemoveItem(object,1,RE::ITEM_REMOVE_REASON::kRemove,instance,nullptr);
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
