#include "ItemIdentity.h"
#include "Presets.h"
#include "FollowerOutfits.h"
#include "FollowerAutoOutfits.h"
#include "StudioSafety.h"
#include "FollowerOutfitState.h"
#include "ManagedItems.h"
#include <mutex>
#include <map>
#include <set>
#include <atomic>

namespace Gallery {
const char* FollowerUnsafeReason(RE::Actor* actor) {
    if(!actor) return FollowerStateReason({.unavailable=true});
    const auto* state=actor->AsActorState();
    return FollowerStateReason({!actor->Get3D() || actor->IsDisabled(), actor->IsDead(),
        actor->IsInKillMove(), actor->IsInRagdollState(), state->IsUnconscious(),
        actor->IsOnMount(), state->IsSwimming(), actor->IsInMidair()});
}
bool IsCurrentOutfitFollower(RE::Actor* actor) {
    if(!actor || actor==RE::PlayerCharacter::GetSingleton()) return false;
    const bool teammate=actor->IsPlayerTeammate();
    bool sofiaMatch=false, forms=false, hired=false, dismissed=false;
    if(auto* data=RE::TESDataHandler::GetSingleton()) {
        constexpr auto plugin="SofiaFollower.esp";
        sofiaMatch=actor==data->LookupForm<RE::Actor>(0x001827,plugin);
        if(sofiaMatch) {
            auto* hireFaction=data->LookupForm<RE::TESFaction>(0x060480,plugin);
            auto* dismissFaction=data->LookupForm<RE::TESFaction>(0x060481,plugin);
            forms=hireFaction && dismissFaction;
            hired=hireFaction && actor->IsInFaction(hireFaction);
            dismissed=dismissFaction && actor->IsInFaction(dismissFaction);
        }
    }
    const bool fallback=sofiaMatch && forms && hired && !dismissed;
    const bool accepted=teammate || fallback;
    return accepted;
}

namespace {
constexpr std::uint32_t record=0x464F5554; // FOUT, actor + count + armor IDs
std::mutex stateMutex;
std::map<RE::FormID,FollowerOutfitState> outfits;
std::atomic<bool> hasOutfits{};

std::atomic<ULONGLONG> nextPoll{};

bool Eligible(RE::Actor* actor) {
    return IsCurrentOutfitFollower(actor) && !FollowerUnsafeReason(actor) && !actor->IsInCombat();
}
Item ArmorItem(RE::TESObjectARMO* armor) {
    auto* file=armor?ItemOwner(armor):nullptr;
    if(!file || (armor->GetFormID()>>24)==0xFF) throw std::runtime_error("Cannot save a dynamically generated item");
    return {std::string(file->GetFilename()),LocalFormID(armor->GetFormID()),armor->GetName()?armor->GetName():"","armor",""};
}
Preset Desired(const FollowerOutfitState& state) {
    Preset p; p.name="Maintained follower outfit";
    for(auto id:state.armor) {
        auto* armor=RE::TESForm::LookupByID<RE::TESObjectARMO>(id);
        if(!armor) throw std::runtime_error("Maintained follower armor is unavailable");
        p.items.push_back(ArmorItem(armor));
    }
    return p;
}
void Maintain() {
    auto* ui=RE::UI::GetSingleton();
    if(!ui || ui->GameIsPaused()) return;
    std::map<RE::FormID,FollowerOutfitState> copy;
    {std::scoped_lock lock(stateMutex); copy=outfits;}
    const auto now=GetTickCount64();
    for(auto& [id,state]:copy) {
        if(now<state.nextCheck || FollowerAutomaticOwns(id)) continue;
        // Resolve on the game thread each time; never keep an actor pointer.
        auto* actor=RE::TESForm::LookupByID<RE::Actor>(id);
        if(!Eligible(actor)) continue;
        try {
            const auto desired=Desired(state);
            bool matches=false;
            try { matches=SameEquipment(desired,SnapshotEquipment(actor)); }
            catch(const std::exception&) { /* no worn armor can be repaired */ }
            const bool correct=state.Observe(matches,now);
            if(!matches && !correct) throw std::runtime_error("Repeated outfit changes; another system may control this follower");
            if(correct) {
                // Never respawn an item handed away or lost. Failure releases
                // maintenance instead of generating replacement inventory.
                ApplyEquipment(actor,desired,false);
                if(SameEquipment(desired,SnapshotEquipment(actor))) ReclaimManagedItems(actor);
                SKSE::log::info("Reapplied maintained outfit to {:08X}",id);
            }
            {std::scoped_lock lock(stateMutex); if(outfits.contains(id)) outfits[id]=std::move(state);}
        } catch(const std::exception& e) {
            ReleaseFollowerOutfit(actor);
            SKSE::log::warn("Follower outfit maintenance disabled for {:08X}: {}",id,e.what());
        }
    }
}
}
unsigned MaintainedFollowerCount() {
    std::scoped_lock lock(stateMutex); return static_cast<unsigned>(outfits.size());
}
void ClearFollowerOutfits() {
    nextPoll=0;
    std::scoped_lock lock(stateMutex); outfits.clear(); hasOutfits=false;
}
bool IsFollowerOutfitMaintained(RE::Actor* actor) {
    if(!actor) return false;
    std::scoped_lock lock(stateMutex); return outfits.contains(actor->GetFormID());
}
void RememberFollowerOutfit(RE::Actor* actor) {
    if(!Eligible(actor)) throw std::runtime_error("Follower is not available for outfit maintenance.");
    FollowerOutfitState state;
    // Validate a restorable armor-only snapshot, including library persistence.
    const auto snapshot=SnapshotEquipment(actor);
    for(const auto& item:snapshot.items) {
        auto* form=ResolveItem(item);
        if(!form || !form->As<RE::TESObjectARMO>()) throw std::runtime_error("Maintained follower armor is unavailable");
        state.armor.push_back(form->GetFormID());
    }
    state.nextCheck=GetTickCount64()+5000;
    std::scoped_lock lock(stateMutex);
    if(outfits.size()>=256 && !outfits.contains(actor->GetFormID())) throw std::runtime_error("Too many maintained followers.");
    outfits[actor->GetFormID()]=std::move(state); hasOutfits=true;
}
void ReleaseFollowerOutfit(RE::Actor* actor) {
    if(!actor) return;
    std::scoped_lock lock(stateMutex); outfits.erase(actor->GetFormID()); hasOutfits=!outfits.empty();
}
void RefreshFollowerOutfit(RE::Actor* actor) {
    if(actor && !FollowerAutomaticOwns(actor->GetFormID()) && IsFollowerOutfitMaintained(actor)) RememberFollowerOutfit(actor);
}
bool FollowerMaintenanceDue() {return hasOutfits && GetTickCount64()>=nextPoll.load();}
void MaintainFollowerOutfits() {
    if(!FollowerMaintenanceDue()) return;
    nextPoll=GetTickCount64()+1000;
    Maintain();
}
void SaveFollowerOutfits(SKSE::SerializationInterface* s) {
    std::map<RE::FormID,FollowerOutfitState> copy;
    {std::scoped_lock lock(stateMutex); copy=outfits;}
    for(const auto& [actor,state]:copy) {
        const auto count=static_cast<std::uint32_t>(state.armor.size());
        if(!s->OpenRecord(record,1) || !s->WriteRecordData(actor) || !s->WriteRecordData(count)) {
            SKSE::log::error("Could not save follower outfit state"); return;
        }
        for(auto id:state.armor) if(!s->WriteRecordData(id)) {
            SKSE::log::error("Could not save follower armor IDs"); return;
        }
    }
}
bool LoadFollowerOutfit(SKSE::SerializationInterface* s,std::uint32_t type,std::uint32_t version,std::uint32_t length) {
    if(type!=record) return false;
    if(version!=1 || length<12 || length>8+4*256 || length%4) return true;
    std::vector<std::uint32_t> data(length/4);
    if(s->ReadRecordData(data.data(),length)!=length) return true;
    auto decoded=DecodeFollowerOutfit(data,[s](auto old,auto& current){return s->ResolveFormID(old,current);});
    if(!decoded) {SKSE::log::warn("Skipped unresolved or invalid follower outfit record"); return true;}
    auto& [actor,armor]=*decoded;
    FollowerOutfitState state; state.armor=std::move(armor);
    std::scoped_lock lock(stateMutex);
    if(outfits.size()<256) {outfits[actor]=std::move(state); hasOutfits=true;}
    return true;
}
Preset DefaultFollowerOutfit(RE::Actor* actor) {
    if(!Eligible(actor)) throw std::runtime_error("Follower is not available for outfit maintenance.");
    auto* base=actor->GetActorBase();
    auto* outfit=base?base->defaultOutfit:nullptr;
    if(!outfit) throw std::runtime_error("No standard outfit is defined for this follower.");
    std::set<RE::TESObjectARMO*> armor;
    for(auto* item:outfit->outfitItems) if(item) {
        if(auto* piece=item->As<RE::TESObjectARMO>()) armor.insert(piece);
        else if(item->As<RE::TESLevItem>()) throw std::runtime_error("Standard outfit uses leveled items; this feature cannot safely restore it.");
    }
    // Include already instantiated hidden outfit armor. Never roll a leveled
    // list, reset the inventory, or rewrite the shared NPC base.
    for(const auto& [object,data]:actor->GetInventory()) {
        auto* piece=object?object->As<RE::TESObjectARMO>():nullptr;
        if(!piece || data.first<=0 || !data.second || !data.second->extraLists) continue;
        for(auto* extra:*data.second->extraLists) if(extra) {
            auto* tag=extra->GetByType<RE::ExtraOutfitItem>();
            if(tag && tag->id==outfit->GetFormID()) armor.insert(piece);
        }
    }
    Preset result; result.name="Standard follower outfit";
    for(auto* piece:armor) result.items.push_back(ArmorItem(piece));
    if(result.items.empty()) throw std::runtime_error("No armor was found in this follower's standard outfit.");
    return result;
}
}
