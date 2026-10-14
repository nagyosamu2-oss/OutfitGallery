#pragma once
#include <functional>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <array>
#include <optional>
#include <stdexcept>
#include "PreferredItem.h"
namespace RE { class Actor; }
namespace Gallery {
using AutoOutfitSlots=std::array<std::string,27>; // Default, Town, Home, Inn, Combat, Night, Rain, Snow, Dungeon, Temple, Swimming, six guild bases; existing indices stay stable
struct AutoOutfitSettings { AutoOutfitSlots slots; int delaySeconds{3}; int columns{3}; int ostimRecoverySeconds{10}; bool enabled{}, paused{}, combatEnabled{}, hideAssignButtons{}; int sneakDelaySeconds{2}; int combatIntervalSeconds{5}; };
AutoOutfitSettings ReadAutoOutfitSettings(const std::filesystem::path&);
void WriteAutoOutfitSettings(const AutoOutfitSettings&,const std::filesystem::path&);
struct InputSettings { unsigned keyboard{66}, gamepad{32}; float holdSeconds{0.8f}; std::array<unsigned,3> captureKeys{67,68,87}; std::array<unsigned,3> capturePads{32768,16384,64}; unsigned keyboardModifiers{}; unsigned autoKeyboard{}, autoModifiers{}, autoGamepad{}; float autoHoldSeconds{.8f}; };
InputSettings ReadInputSettings(const std::filesystem::path&);
void WriteInputSettings(const InputSettings&,const std::filesystem::path&);
struct StudioSettings { float distance{217.130f}, height{67.619f}, orbit{-22.831f}, pitch{0.350f}, fov{60.060f}; int columns{3}; bool showNames{true}, showCounts{true}; int language{0}; bool allowFreeCamera{}; bool addMissing{true}; float lateral{13.929f}, elevation{0.000f}; std::uint32_t headSlots{0x1803}; std::string startupTab{}; bool detachedPreview{}; std::array<float,4> previewRect{.54f,.08f,.45f,.84f}; bool followerTargeting{}; bool preferEnchanted{true}; bool portraitThumbnails{}; bool cleanupCompatibility{}; bool allowCombatGallery{}; bool compactGallery{}; };
struct CameraBank {
    std::array<std::optional<StudioSettings>,7> slots;
    std::array<std::string,7> names;
    auto& operator[](std::size_t n) {return slots[n];}
    const auto& operator[](std::size_t n) const {return slots[n];}
    auto begin() const {return slots.begin();}
    auto end() const {return slots.end();}
};
CameraBank ReadCameraBank(const std::filesystem::path&);
void WriteCameraBank(const CameraBank&,const std::filesystem::path&);
struct Category { std::string id, name; };
struct PresetLabel { std::string name; std::vector<std::string> categories; bool deleted{}; std::optional<std::uint32_t> exchangeSlots; std::vector<std::string> exchangeSlotless; std::optional<std::vector<std::string>> outfitItems; };
struct Library { std::vector<Category> categories; std::map<std::string,PresetLabel> labels; std::vector<std::string> order; };
inline std::string ResolveStartupTab(const std::string& preferred,const Library& library) {
    if(preferred.empty() || preferred=="#head" || preferred=="#accessories" || preferred=="#trash") return preferred;
    bool hasFavorites=false;
    for(const auto& c:library.categories) {
        if(c.id==preferred) return preferred;
        if(c.id=="favorites") hasFavorites=true;
    }
    return hasFavorites?"favorites":"";
}
bool MovePhoto(std::vector<std::string>& order,const std::string& source,const std::string& target,bool after);
using DeletionTrace=std::function<void(const std::string&)>;
void DeleteTrashedFiles(const std::string& photo,const Library&,const std::filesystem::path& root,DeletionTrace trace={});
Library ReadLibrary(const std::filesystem::path&);
void WriteLibrary(const Library&,const std::filesystem::path&);
StudioSettings ReadStudioSettings(const std::filesystem::path&);
void WriteStudioSettings(const StudioSettings&, const std::filesystem::path&);
struct Item { std::string plugin; std::uint32_t localID{}; std::string name; std::string kind; std::string hand; PreferredItem preferred; };
struct Preset { std::string name; std::string photo; std::vector<Item> items; std::uint32_t slotMask{}; bool accessories{}; std::optional<std::uint32_t> exchangeSlots; bool equipmentEdited{}; std::vector<std::string> exchangeSlotless; std::optional<std::vector<std::string>> outfitItems; };
inline std::string ExchangeItemKey(const Item& item) {return item.plugin+":"+std::to_string(item.localID);}
// Old presets may contain weapons/ammo. Keep their files readable, but never
// apply those entries or include them in post-apply verification.
inline Preset ClothingPreset(const Preset& source) {
    auto result=source;
    result.items.clear();
    for(const auto& item:source.items) if(item.kind=="armor") result.items.push_back(item);
    return result;
}
inline bool SlotIntersects(std::uint32_t item, std::uint32_t scope) {return (item & scope)!=0;}
inline bool SlotFits(std::uint32_t item, std::uint32_t scope) {return item && (item & ~scope)==0;}
inline bool ReplaceWornForScope(std::uint32_t worn,std::uint32_t scope) {
    return SlotIntersects(worn,scope);
}
inline bool AccessoryItemInScope(std::uint32_t worn,std::uint32_t scope,bool registered) {
    return SlotIntersects(worn,scope) || (!worn && registered);
}
inline bool CrossesExchangeBoundary(std::uint32_t worn,std::uint32_t scope) {
    return SlotIntersects(worn,scope) && !SlotFits(worn,scope);
}
inline std::uint32_t ToggleExchangeSlot(std::uint32_t selection,std::uint32_t bit,bool checked,const std::vector<std::uint32_t>& masks) {
    if(checked) selection|=bit; else selection&=~bit;
    for(unsigned pass=0;pass<32;++pass) {
        const auto before=selection;
        for(auto mask:masks) {
            if(checked && (mask&selection)) selection|=mask;
            else if(!checked && CrossesExchangeBoundary(mask,selection)) selection&=~mask;
        }
        if(before==selection) break;
    }
    return selection;
}
inline bool CollectionFits(std::uint32_t scope,const std::string& category,bool accessories=false) {
    if(category=="#trash") return true;
    if(category=="#accessories") return accessories;
    if(accessories) return false;
    if(category=="#head") return SlotFits(scope,0x1803);
    if(category=="#legacy") return scope && !SlotFits(scope,0x1803);
    return scope==0;
}
// Full outfits expose the union of saved armor slots without changing their
// original zero (full replacement) scope. Partial outfits retain their scope.
inline std::uint32_t AvailableExchangeSlots(const Preset& source,const std::vector<std::uint32_t>& masks) {
    if(masks.size()!=source.items.size()) throw std::runtime_error("Invalid exchange slots.");
    if(source.slotMask) return source.slotMask;
    std::uint32_t available=0;
    for(std::size_t n=0;n<masks.size();++n) if(source.items[n].kind=="armor") available|=masks[n];
    return available;
}
// Pure scope policy; no inventory mutation. Zero override always means no-op.
inline Preset FilterExchangeSlots(const Preset& source,const std::vector<std::uint32_t>& masks) {
    if(!source.slotMask && !source.accessories && source.outfitItems) {
        auto result=source; result.items.clear();
        for(const auto& item:source.items) if(item.kind=="armor" &&
            std::find(source.outfitItems->begin(),source.outfitItems->end(),ExchangeItemKey(item))!=source.outfitItems->end()) result.items.push_back(item);
        return result;
    }
    if(!source.exchangeSlots) return source;
    if(*source.exchangeSlots & ~AvailableExchangeSlots(source,masks))
        throw std::runtime_error("Invalid exchange slots.");
    auto result=source; const auto selectedMask=*source.exchangeSlots; result.slotMask=(source.slotMask || source.accessories)?selectedMask:0; result.items.clear();
    for(std::size_t n=0;n<masks.size();++n) {
        if(source.items[n].kind!="armor") continue;
        if(!masks[n] && std::find(source.exchangeSlotless.begin(),source.exchangeSlotless.end(),ExchangeItemKey(source.items[n]))!=source.exchangeSlotless.end()) {
            result.items.push_back(source.items[n]);if(source.slotMask || source.accessories) result.accessories=true;continue;
        }
        if(!SlotIntersects(masks[n],selectedMask)) continue;
        if(!SlotFits(masks[n],selectedMask)) throw std::runtime_error("Select all slots used by each item.");
        result.items.push_back(source.items[n]);
    }
    if((selectedMask || !source.exchangeSlotless.empty()) && result.items.empty()) throw std::runtime_error("No saved items in the selected slots.");
    return result;
}
std::vector<std::uint32_t> ResolvePresetSlots(const Preset&); // game thread only
bool SameEquipment(const Preset&,const Preset&);
void WritePreset(const Preset&, const std::filesystem::path&);
Preset ReadPreset(const std::filesystem::path&);
Preset SnapshotForVerification(RE::Actor*,const Preset&);
Preset SnapshotEquipment(RE::Actor* actor, std::uint32_t slotMask=0, bool rememberEnchanted=false); // game thread only; zero = full outfit
Preset SnapshotAccessories(RE::Actor* actor, bool rememberEnchanted=false); // equipped armor only, union of occupied slots
std::string ApplyEquipment(RE::Actor* actor, const Preset&, bool addMissing, bool preferEnchanted=false,
    const std::function<bool()>& mayMutate={}); // game thread only; optional crafting deadline
std::string VerifyPreferredEquipment(RE::Actor*, const Preset&); // worn identity only; does not mutate inventory
}



