#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "TransmogFailure.h"
namespace Gallery {
// EXE file versions, not SKSE's packed versions: GOG's store nibble (1)
// exists in SKSEPlugin_Version / RuntimeVersion(), but not Module::version().
inline constexpr std::array<std::array<std::uint16_t,4>,7> TransmogRuntimes{{
    {1,5,97,0},{1,6,353,0},{1,6,640,0},{1,6,1130,0},{1,6,1170,0},{1,7,104,0},{1,6,1179,0}}};
struct TransmogHookOffsets { std::uintptr_t mask{},worn{}; };
constexpr TransmogHookOffsets MeasuredTransmogOffsets(const std::array<std::uint16_t,4>& v) {
    // Verified layouts only. Other runtimes keep callee-based discovery; do
    // not guess a foreign hook's call site from a neighboring game version.
    if(v==std::array<std::uint16_t,4>{1,5,97,0}) return {0x7C,0x81};
    if(v==std::array<std::uint16_t,4>{1,6,1170,0}) return {0x80,0x1EF};
    if(v==std::array<std::uint16_t,4>{1,7,104,0}) return {0x80,0x1EF};
    return {};
}
struct TransmogRuntimeSites {
    std::uintptr_t maskSite{},maskTarget{},visitSite{},visitTarget{},visitorTable{},stage{},refreshFlag{},refreshModels{};
    std::uintptr_t maskCall{},visitCall{};
    std::string maskModule,visitModule;
};
std::optional<TransmogRuntimeSites> LocateTransmogRuntime(TransmogFailure& failure);
}
