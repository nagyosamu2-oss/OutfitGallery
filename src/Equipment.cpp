#include "ItemIdentity.h"
#include "Presets.h"
#include "Diagnostics.h"
#include "ManagedItems.h"
#include "EnchantmentSignature.h"
#include <map>
#undef GetObject
namespace Gallery {
namespace {
std::string Kind(RE::TESBoundObject* o) {
    if (o->As<RE::TESObjectARMO>()) return "armor";
    if (o->As<RE::TESObjectWEAP>()) return "weapon";
    if (o->As<RE::TESAmmo>()) return "ammo";
    return {};
}
const RE::BGSEquipSlot* Slot(const std::string& hand) {
    if (hand.empty()) return nullptr;
    return RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
        hand == "left" ? RE::DEFAULT_OBJECT::kLeftHandEquip : RE::DEFAULT_OBJECT::kRightHandEquip);
}
Item Encode(RE::TESBoundObject* obj, const std::string& hand) {
    auto* file = ItemOwner(obj);
    if (!file || (obj->GetFormID() >> 24) == 0xFF) throw std::runtime_error("Cannot save a dynamically generated item");
    // Strip the load index, including the ESL light-plugin index.
    const auto id = LocalFormID(obj->GetFormID());
    return {std::string(file->GetFilename()),id,obj->GetName() ? obj->GetName() : "",Kind(obj),hand};
}
}
std::vector<std::uint32_t> ResolvePresetSlots(const Preset& p) {
    auto* handler=RE::TESDataHandler::GetSingleton();
    if(!handler) throw std::runtime_error("Equipment manager unavailable");
    std::vector<std::uint32_t> masks;
    for(const auto& i:p.items) {
        if(i.kind!="armor") {masks.push_back(0); continue;} // Legacy weapon/ammo entries are never applied.
        auto* form=ResolveItem(i);
        auto* armor=form?form->As<RE::TESObjectARMO>():nullptr;
        if(!armor) throw std::runtime_error("Missing or changed mod item: "+i.plugin+" / "+i.name);
        const auto mask=armor->GetSlotMask().underlying();
        if((p.slotMask || p.accessories) && !(p.accessories && !mask) && !SlotFits(mask,p.slotMask)) throw std::runtime_error("Item uses unselected slots: "+i.name);
        masks.push_back(mask);
    }
    return masks;
}
Preset SnapshotEquipment(RE::Actor* actor, std::uint32_t slotMask, bool rememberEnchanted, bool rememberNames) {
    if (!actor) throw std::runtime_error("Target unavailable");
    Preset p;
    p.slotMask=slotMask;
    for (const auto& [obj, data] : actor->GetInventory()) {
        const auto& [count, entry] = data;
        if (!obj || count <= 0 || !entry || !entry->IsWorn() || !obj->As<RE::TESObjectARMO>()) continue;
        if(slotMask) {
            const auto* armor=obj->As<RE::TESObjectARMO>();
            if(!armor || !SlotIntersects(armor->GetSlotMask().underlying(),slotMask)) continue;
            if(!SlotFits(armor->GetSlotMask().underlying(),slotMask)) throw std::runtime_error("Item uses unselected slots: "+std::string(obj->GetName()));
        }
        p.items.push_back(Encode(obj,""));
        if(rememberNames && cleanupCompatibility.load()) p.items.back().registeredName=CaptureRegisteredName(obj,data.second.get());
        if(rememberEnchanted && actor->GetFormID()==0x14 && entry->extraLists) {
            RE::ExtraDataList* worn=nullptr; unsigned countWorn=0;
            for(auto* e:*entry->extraLists) if(e && (e->HasType<RE::ExtraWorn>() || e->HasType<RE::ExtraWornLeft>())) {worn=e; ++countWorn;}
            if(countWorn==1) p.items.back().preferred=RememberEnchanted(actor,worn);
        }
    }
    if (p.items.empty()) throw std::runtime_error("No equipped armor or clothing to save");
    return p;
}
Preset SnapshotAccessories(RE::Actor* actor, bool rememberEnchanted, bool rememberNames) {
    if(!actor) throw std::runtime_error("Target unavailable");
    Preset p; p.accessories=true;
    for(const auto& [obj,data]:actor->GetInventory()) {
        if(!obj || data.first<=0 || !data.second || !data.second->IsWorn()) continue;
        const auto* armor=obj->As<RE::TESObjectARMO>();
        if(!armor) continue;
        const auto mask=armor->GetSlotMask().underlying();
        p.slotMask|=mask; p.items.push_back(Encode(obj,""));
        if(rememberNames && cleanupCompatibility.load()) p.items.back().registeredName=CaptureRegisteredName(obj,data.second.get());
        if(rememberEnchanted && actor->GetFormID()==0x14 && data.second->extraLists) {
            RE::ExtraDataList* worn=nullptr; unsigned countWorn=0;
            for(auto* e:*data.second->extraLists) if(e && (e->HasType<RE::ExtraWorn>() || e->HasType<RE::ExtraWornLeft>())) {worn=e; ++countWorn;}
            if(countWorn==1) p.items.back().preferred=RememberEnchanted(actor,worn);
        }
    }
    if(p.items.empty()) throw std::runtime_error("No equipped accessories to save.");
    return p;
}
Preset SnapshotForVerification(RE::Actor* actor,const Preset& target) {
    if(!target.accessories) return SnapshotEquipment(actor,target.slotMask);
    auto actual=SnapshotEquipment(actor);
    const auto masks=ResolvePresetSlots(actual);
    Preset selected=target;selected.items.clear();
    for(std::size_t n=0;n<actual.items.size();++n) {
        const auto& item=actual.items[n];
        bool registered=false;
        for(const auto& expected:target.items) {
            Preset a,b;a.items={item};b.items={expected};
            if(SameEquipment(a,b)) {registered=true;break;}
        }
        if(AccessoryItemInScope(masks[n],target.slotMask,registered)) selected.items.push_back(item);
    }
    return selected;
}
std::string VerifyPreferredEquipment(RE::Actor* actor,const Preset& p) {
    if(!actor || actor->GetFormID()!=0x14 || !std::any_of(p.items.begin(),p.items.end(),[](const Item& i){return i.preferred.custom;})) return {};
    const auto inventory=actor->GetInventory(); std::string missing;
    for(const auto& item:p.items) {
        if(!item.preferred.custom) continue;
        auto* form=ResolveItem(item);
        auto* obj=form?form->As<RE::TESBoundObject>():nullptr;
        const auto found=inventory.find(obj);
        std::vector<PreferredCandidate> candidates; std::vector<bool> worn;
        if(found!=inventory.end() && found->second.first>0 && found->second.second && found->second.second->extraLists)
            for(auto* e:*found->second.second->extraLists) if(e) {
                auto* uid=e->GetByType<RE::ExtraUniqueID>();
                candidates.push_back({uid?uid->uniqueID:std::uint16_t{},uid && uid->baseID==0x14,e->HasType<RE::ExtraEnchantment>(),e->GetCount(),item.preferred.signature.empty()?std::string{}:EnchantmentSignature(e)});
                worn.push_back(e->HasType<RE::ExtraWorn>() || e->HasType<RE::ExtraWornLeft>());
            }
        const auto index=FindPreferred(item.preferred,PreferredScope(item.preferred.id),candidates);
        const bool equipped=index>=0 && worn[index];
        if(!equipped) {if(!missing.empty()) missing+=", "; missing+=item.name;}
    }
    return missing.empty()?std::string{}:" Registered enchantment not restored: "+missing;
}
std::string ApplyEquipment(RE::Actor* actor, const Preset& source, bool addMissing, bool preferEnchanted,
    const std::function<bool()>& mayMutate) {
    if(actor && actor->GetFormID()!=0x14)
        GALLERY_DIAG("OG-ARRIVAL-DIAG og-apply-enter tick={} actor={:08X} photo={} preset={}",GetTickCount64(),actor->GetFormID(),source.photo,source.name);
    const auto checkDeadline=[&] {
        if(mayMutate && !mayMutate())
            throw std::runtime_error("Crafting menu or scene started during equipment preparation; stopped until next use");
    };
    checkDeadline();
    const bool compatibility=cleanupCompatibility.load(); // snapshot for the whole transaction
    CleanupTiming timing{"apply"};
    GALLERY_DIAG("OG-DIAG apply preset={} photo={} actor={:08X} compatibility={} addMissing={} preferEnchanted={}",source.name,source.photo,actor?actor->GetFormID():0,compatibility,addMissing,preferEnchanted);
    if(source.exchangeSlots && !*source.exchangeSlots && source.exchangeSlotless.empty()) return "No exchange slots selected. Equipment unchanged.";
    const auto p=ClothingPreset(source);
    if(p.items.empty()) throw std::runtime_error("No equipped armor or clothing to save");
    BeginManagedAddition(actor);
    auto* manager = RE::ActorEquipManager::GetSingleton();
    auto* handler = RE::TESDataHandler::GetSingleton();
    if (!actor || !manager || !handler) throw std::runtime_error("Equipment manager unavailable");
    std::vector<std::pair<RE::TESBoundObject*,const RE::BGSEquipSlot*>> resolved;
    std::map<RE::TESBoundObject*,int> needed;
    std::uint32_t resolvedMask=0;
    // Resolve every item before changing inventory or removing any equipment.
    for (const auto& i : p.items) {
        auto* form = ResolveItem(i);
        auto* obj = form ? form->As<RE::TESBoundObject>() : nullptr;
        if (!obj || Kind(obj) != i.kind) throw std::runtime_error("Missing or changed mod item: " + i.plugin + " / " + i.name);
        if(p.slotMask || p.accessories) {
            const auto* armor=obj->As<RE::TESObjectARMO>();
            if(!armor || (!(p.accessories && !armor->GetSlotMask().underlying()) && !SlotFits(armor->GetSlotMask().underlying(),p.slotMask))) throw std::runtime_error("Item uses unselected slots: "+i.name);
            resolvedMask|=armor->GetSlotMask().underlying();
        }
        auto* slot = Slot(i.hand);
        if (!i.hand.empty() && !slot) throw std::runtime_error("Hand equip slot unavailable");
        GALLERY_DIAG("OG-DIAG resolve plugin={} localID={:06X} form={:08X} name={}",i.plugin,i.localID,obj->GetFormID(),i.name);
        resolved.emplace_back(obj,slot); ++needed[obj];
    }
    if(p.accessories && resolvedMask!=p.slotMask) throw std::runtime_error("Accessory slots changed. Register this set again.");
    auto inventory = actor->GetInventory();
    // The selected preset wins slot conflicts. An overlapping worn armor piece
    // is removed as a whole, even when it also occupies slots outside this scope.
    // Incoming items still must fit the saved scope (validated above).
    for (const auto& [obj, count] : needed) {
        const auto found = inventory.find(obj);
        const auto owned = found == inventory.end() ? 0 : std::max(0,found->second.first);
        if (owned < count && !addMissing) throw std::runtime_error("Not owned: " + std::string(obj->GetName()) + ". Enable Add missing base items for a new game.");
    }
    for (const auto& [obj, count] : needed) {
        const auto found = inventory.find(obj);
        const auto owned = found == inventory.end() ? 0 : std::max(0,found->second.first);
        GALLERY_DIAG("OG-DIAG inventory form={:08X} owned={} needed={}",obj->GetFormID(),owned,count);
        if(owned<count) {
            checkDeadline();
            if(mayMutate) SKSE::log::info("OG-CRAFT generate form={:08X} count={} name={}",obj->GetFormID(),count-owned,obj->GetName());
            RegisteredName name;
            if(compatibility && owned==0 && count==1) {
                for(std::size_t n=0;n<resolved.size();++n) if(resolved[n].first==obj) name=p.items[n].registeredName;
            }
            AddManagedItems(actor,obj,count-owned,owned==0,name);
        }
    }
    checkDeadline();
    if(compatibility) ObserveGeneratedItems(actor);
    // Crafting can now generate items. Do not reuse pre-add extra-list pointers.
    if(mayMutate) inventory=actor->GetInventory();
    checkDeadline();
    // Use each worn instance when unequipping; never destroy inventory items.
    for (const auto& [obj,data] : inventory) {
        const auto& entry = data.second;
        if (!obj || !entry || !entry->extraLists || !obj->As<RE::TESObjectARMO>()) continue;
        if(p.slotMask || p.accessories) {
            const auto* armor=obj->As<RE::TESObjectARMO>();
            if(!armor || !(p.accessories?AccessoryItemInScope(armor->GetSlotMask().underlying(),p.slotMask,needed.contains(obj)):ReplaceWornForScope(armor->GetSlotMask().underlying(),p.slotMask))) continue;
        }
        for (auto* extra : *entry->extraLists) {
            if (!extra) continue;
            const bool right = extra->HasType<RE::ExtraWorn>();
            const bool left = extra->HasType<RE::ExtraWornLeft>();
            if(right || left) checkDeadline();
            if (right) manager->UnequipObject(actor,obj,extra,1,obj->As<RE::TESObjectWEAP>() ? Slot("right") : nullptr,false,false,false,true);
            // The first unequip may destroy its extra list; do not reuse that
            // pointer if an instance happened to carry both worn markers.
            if (left) {
                checkDeadline();
                manager->UnequipObject(actor,obj,right ? nullptr : extra,1,obj->As<RE::TESObjectWEAP>() ? Slot("left") : nullptr,false,false,false,true);
            }
        }
    }
    const bool search=preferEnchanted && actor->GetFormID()==0x14 && std::any_of(p.items.begin(),p.items.end(),[](const Item& i){return i.preferred.custom;});
    // Unequip may destroy/rebuild extra lists. Obtain fresh entries only AFTER
    // all unequips; never carry an ExtraDataList pointer across those mutations.
    std::string missing;
    if(search) {
        auto fresh=actor->GetInventory();
        for(std::size_t n=0;n<resolved.size();++n) {
            const auto [obj,slot]=resolved[n]; auto* selected=static_cast<RE::ExtraDataList*>(nullptr);
            const auto& pref=p.items[n].preferred;
            if(pref.custom) {
                std::vector<PreferredCandidate> candidates; std::vector<RE::ExtraDataList*> lists;
                const auto found=fresh.find(obj);
                if(found!=fresh.end() && found->second.first>0 && found->second.second && found->second.second->extraLists) {
                    for(auto* e:*found->second.second->extraLists) if(e) {
                        auto* uid=e->GetByType<RE::ExtraUniqueID>();
                        candidates.push_back({uid?uid->uniqueID:std::uint16_t{},uid && uid->baseID==0x14,e->HasType<RE::ExtraEnchantment>(),e->GetCount(),pref.signature.empty()?std::string{}:EnchantmentSignature(e)}); lists.push_back(e);
                    }
                }
                const auto index=FindPreferred(pref,PreferredScope(pref.id),candidates);
                if(index>=0) {selected=lists[index];}
                else {if(!missing.empty()) missing+=", "; missing+=p.items[n].name;}
            }
            checkDeadline();
            manager->EquipObject(actor,obj,selected,1,slot,false,false,false,true);
        }
    } else {
        for (const auto& [obj,slot] : resolved) {
            checkDeadline();
            manager->EquipObject(actor,obj,nullptr,1,slot,false,false,false,true);
        }
    }
    TrackNewManagedItems(actor,compatibility);
    // The blocking gallery can defer the normal actor model refresh until close.
    // Refresh once after the complete outfit, on the game task thread. CommonLib
    // also dispatches NiNodeUpdateEvent for consumers such as appearance plugins.
    actor->Update3DModel();
    if(actor->GetFormID()!=0x14)
        GALLERY_DIAG("OG-ARRIVAL-DIAG og-apply-return tick={} actor={:08X}",GetTickCount64(),actor->GetFormID());
    return "Equip requests sent: " + p.name + ". Check the preview."+(missing.empty()?std::string{}:" Registered enchantment not restored: "+missing);
}
}

namespace Gallery {
void RememberSelectedEnchanted(RE::Actor* actor,Preset& selected,bool remember) {
    if(!actor) throw std::runtime_error("Target unavailable");
    const auto inventory=actor->GetInventory();
    for(auto& item:selected.items) {
        auto* form=ResolveItem(item);
        auto* object=form?form->As<RE::TESObjectARMO>():nullptr;
        const auto found=object?inventory.find(object):inventory.end();
        if(found==inventory.end() || found->second.first<=0 || !found->second.second || !found->second.second->IsWorn())
            throw std::runtime_error("Worn equipment changed. Reopen the editor.");
        auto* entry=found->second.second.get();
        item.preferred={};
        if(actor->GetFormID()==0x14 && entry->extraLists) {
            RE::ExtraDataList* worn=nullptr;unsigned count=0;
            for(auto* extra:*entry->extraLists) if(extra && (extra->HasType<RE::ExtraWorn>() || extra->HasType<RE::ExtraWornLeft>())) {worn=extra;++count;}
            if(count!=1) throw std::runtime_error("Worn equipment identity is ambiguous.");
            if(remember) item.preferred=RememberEnchanted(actor,worn);
            else if(worn->HasType<RE::ExtraEnchantment>()) {item.preferred.custom=true;item.preferred.signature=EnchantmentSignature(worn);}
        }
    }
}
}
