#pragma once
#include <algorithm>
#include <cmath>
namespace Gallery {
inline float NormalizeUiZoom(float value) {
    return std::isfinite(value) ? std::clamp(value,.75f,2.f) : 1.f;
}
// Negative means use the framework theme unchanged, including after reset.
inline float NormalizeUiBackgroundOpacity(float value) {
    return std::isfinite(value) && value>=0.f ? std::clamp(value,0.f,1.f) : -1.f;
}
inline float ResolveUiBackgroundOpacity(float value,float themeAlpha) {
    const float normalized=NormalizeUiBackgroundOpacity(value);
    return normalized<0.f ? std::clamp(themeAlpha,0.f,1.f) : normalized;
}
// Coordinates must match the existing render surface. Scaling a logical
// WQHD canvas into that surface also preserves proportions after upscaling.
// This does not change SMF's render target or its font atlas resolution.
inline float ResolveUiScale(float width,float height,float zoom) {
    if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0) return 1.f;
    return std::clamp(std::min(width/2560.f,height/1440.f),.25f,4.f)*NormalizeUiZoom(zoom);
}
}
