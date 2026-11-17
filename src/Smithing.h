#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <stdexcept>
namespace Gallery {
// Portable recipe: no actor skill, pointer, runtime FormID or dynamic enchantment.
struct SmithingRecipe {
    float health{1.f};
    std::uint64_t scope{};
    std::uint16_t id{};
    bool operator==(const SmithingRecipe&) const = default;
};
inline bool ValidSmithing(float health) {return std::isfinite(health) && health>1.f;}
struct SmithingCandidate {
    std::uint16_t id{};int count{};bool original{},managed{};
};
inline std::uint16_t SelectSmithingCandidate(const std::vector<SmithingCandidate>& candidates) {
    std::uint16_t original{},managed{};unsigned originals{},copies{};
    for(const auto& c:candidates) {
        if((c.original || c.managed) && (!c.id || c.count!=1)) throw std::runtime_error("Smithing identity is ambiguous");
        if(c.original) {original=c.id;++originals;}
        if(c.managed) {managed=c.id;++copies;}
    }
    if(originals>1) throw std::runtime_error("Registered smithing identity is ambiguous");
    if(originals==1) return original;
    if(copies>1) throw std::runtime_error("Multiple managed smithing copies; resolve inventory before retrying");
    return managed;
}
// Transaction evidence only; never an inventory pointer or a persisted claim.
struct SmithingCreation {
    std::uint32_t form{},owner{};
    std::uint16_t initial{},current{};
    unsigned hops{};
    bool invalid{};
    void Change(std::uint32_t object,std::uint32_t oldOwner,std::uint16_t oldID,std::uint32_t newOwner,std::uint16_t newID) {
        if(object!=form || (oldOwner==newOwner && oldID==newID)) return;
        if(invalid || oldOwner!=owner || oldID!=current || newOwner!=owner || !newID || hops>=4) {invalid=true;return;}
        current=newID;++hops;
    }
};
struct SmithingGenerationEvidence {
    bool invalid{},wasPresent{},reserved{},customEnchanted{};
    int before{},after{},instanceCount{};
    unsigned matches{};
    std::uint32_t actor{},owner{};
    std::uint16_t expected{},actual{};
    float requested{1.f},actualHealth{1.f};
};
inline bool VerifiedSmithingGeneration(const SmithingGenerationEvidence& e) {
    return !e.invalid && !e.wasPresent && !e.reserved && !e.customEnchanted && e.before>=0 &&
        static_cast<long long>(e.after)==static_cast<long long>(e.before)+1 && e.matches==1 && e.instanceCount==1 &&
        e.actor && e.owner==e.actor && e.expected && e.actual==e.expected &&
        ValidSmithing(e.requested) && e.requested==e.actualHealth;
}
inline bool CanRetagSmithingGeneration(SmithingGenerationEvidence evidence,std::uint16_t allocated,
    bool allocatedWasReserved,bool allocatedIsOccupied) {
    if(!evidence.reserved || !allocated || allocated==evidence.actual || allocatedWasReserved || allocatedIsOccupied) return false;
    evidence.reserved=false;
    return VerifiedSmithingGeneration(evidence);
}
struct GeneratedPerformance {
    float health{1.f};
    bool reproduction{}, retained{};
};
// Ignore only the notification of our own exact singleton deletion. A manual
// transfer, another identity, another destination or a different count is not it.
inline bool IsOwnReclaimEvent(std::uint32_t expectedForm,std::uint32_t expectedOwner,std::uint16_t expectedID,
    std::uint32_t form,std::uint32_t from,std::uint32_t to,int count,std::uint16_t id) {
    return expectedForm && expectedOwner && expectedID && form==expectedForm && from==expectedOwner && !to && count==1 && (!id || id==expectedID);
}
inline bool ProtectPerformance(float initial,float current,bool enchanted) {
    return !std::isfinite(initial) || !std::isfinite(current) || current>initial || enchanted;
}
// Inventory-list layout changed in AE 1.6.629. Allocate runtime size, then
// invoke CommonLib's engine constructor; never use sizeof(ExtraDataList).
inline std::size_t ExtraListStorageSize(bool post629) {return post629?0x20:0x18;}
}
