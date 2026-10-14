#pragma once
#include <algorithm>
namespace Gallery {
inline int ThumbnailColumns(int normalColumns,bool portrait) {
    normalColumns=std::clamp(normalColumns,2,6);
    return portrait ? (normalColumns*8+2)/5 : normalColumns;
}
struct ThumbnailLayout {float stepX,width,height,uvLeft,uvRight;};
inline ThumbnailLayout MakeThumbnailLayout(float available,float screenWidth,float screenHeight,int normalColumns,bool portrait) {
    const int columns=ThumbnailColumns(normalColumns,portrait);
    const float normalWidth=std::max(32.f,(available-16.f)/std::clamp(normalColumns,2,6)-12.f);
    const float step=(available-16.f)/columns;
    const float width=std::max(32.f,step-12.f);
    const float fraction=std::clamp(width/normalWidth,0.f,1.f);
    // Taller displays retain the 16:9 thumbnail size; extra height reveals
    // more rows instead of stretching every card. Preserve wider-screen layouts.
    const float thumbnailScreenHeight=std::min(screenHeight,std::max(1.f,screenWidth)*9.f/16.f);
    return {step,width,normalWidth*(thumbnailScreenHeight*.84f)/(std::max(1.f,screenWidth)*.45f),(1.f-fraction)*.5f,(1.f+fraction)*.5f};
}
}
