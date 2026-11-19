#pragma once
#include <cstdint>
namespace Gallery {
using NativeHairDecision=bool(*)(std::uint32_t);
void InstallFsmpNativeHair(NativeHairDecision decision);
}
