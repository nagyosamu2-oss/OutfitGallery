#pragma once
#include <cstdint>
namespace Gallery {
inline constexpr std::uint32_t HeadVisibilityBits=(1u<<0)|(1u<<1);
inline constexpr std::uint32_t HeadPartitionBits=(1u<<0)|(1u<<13);
constexpr std::uint32_t HeadPartitionBit(unsigned partition) {
    // Head-part meshes may use the +100 family; UBE/body meshes can use
    // the ordinary biped numbers. Never admit severed-head/cap sections.
    return partition==30 || partition==130 ? 1u :
        partition==43 || partition==143 ? 1u<<13 : 0u;
}
// A multi-slot piece disappears as a whole when any of its declared slots
// is controlled. Its head coverage must disappear with its geometry.
constexpr std::uint32_t HiddenHeadCoverage(std::uint32_t armorSlots,
    std::uint32_t coverage,std::uint32_t scope) {
    return (armorSlots&scope) && !(armorSlots&(1u<<9)) ? coverage : 0u;
}
constexpr std::uint32_t AppearanceHeadMask(std::uint32_t real,
    std::uint32_t appearance,std::uint32_t scope,std::uint32_t hidden) {
    const auto owned=(scope|hidden)&HeadVisibilityBits;
    return (real&~owned)|(appearance&owned);
}
constexpr std::uint32_t HeadPartitionsToRestore(std::uint32_t hidden,
    std::uint32_t appearance) {
    return hidden&~appearance&HeadPartitionBits;
}
}
