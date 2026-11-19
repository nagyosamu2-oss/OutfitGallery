#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
namespace Gallery {
constexpr bool NativeHairPhysicsVisible(std::uint32_t scope,std::uint32_t headCoverage) {
    // Own native hair visibility only where this look owns31 and displays
    // neither a head-covering helmet nor a hair-covering armor addon.
    return (scope&2u) && !(headCoverage&3u);
}
class ScopedHeadPhysics {
    std::uint8_t& flag;
    std::uint8_t previous;
public:
    ScopedHeadPhysics(std::uint8_t& value,bool force):flag(value),previous(value) {
        if(force && previous==0) flag=1;
    }
    ~ScopedHeadPhysics(){flag=previous;}
    ScopedHeadPhysics(const ScopedHeadPhysics&)=delete;
    ScopedHeadPhysics& operator=(const ScopedHeadPhysics&)=delete;
};
inline constexpr std::array<std::uint8_t,10> NativeHairEntry{
    0x48,0x89,0x5c,0x24,0x10, // mov [rsp+10h],rbx
    0x48,0x89,0x6c,0x24,0x18  // mov [rsp+18h],rbp
};
constexpr bool NativeHairNearEnough(std::intptr_t distance) {
    return distance>=std::numeric_limits<std::int32_t>::min() &&
        distance<=std::numeric_limits<std::int32_t>::max();
}
inline std::array<std::uint8_t,24> NativeHairContinuation(std::uint64_t target) {
    std::array<std::uint8_t,24> bytes{};
    for(unsigned i=0;i<NativeHairEntry.size();++i) bytes[i]=NativeHairEntry[i];
    bytes[10]=0xff;bytes[11]=0x25; // RIP-indirect absolute jump; no registers clobbered
    std::memcpy(bytes.data()+16,&target,sizeof(target));
    return bytes;
}
inline std::array<std::uint8_t,10> NativeHairEntryPatch(std::int32_t displacement) {
    std::array<std::uint8_t,10> bytes{0xff,0x25,0,0,0,0,0x90,0x90,0x90,0x90};
    std::memcpy(bytes.data()+2,&displacement,sizeof(displacement));
    return bytes;
}
}
