#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace Gallery::Variations {
struct StripLayout {
    float imageWidth,imageHeight,cardWidth,cardHeight,stride,contentWidth,contentHeight,width,height;
    std::size_t columns;bool scrolling;
};
struct StripArea { float top,height; };
inline StripArea OrganizerStripArea(float anchorTop,float desiredHeight,float viewTop,float viewBottom,float selectionBand,float gap) {
    // Keep the source card's selection marker exposed. Prefer a slight downward
    // offset, but place above at the bottom edge instead of covering it again.
    const float below=std::clamp(anchorTop+selectionBand,viewTop,viewBottom);
    const float above=std::clamp(anchorTop-gap,viewTop,viewBottom);
    if(viewBottom-below>=desiredHeight) return {below,viewBottom-below};
    if(above-viewTop>=desiredHeight) return {above-desiredHeight,desiredHeight};
    if(viewBottom-below>=above-viewTop) return {below,viewBottom-below};
    return {viewTop,above-viewTop};
}
inline float RevealScroll(float scroll,float maximum,int index,float stride,float cardWidth,float viewportWidth) {
    const float left=std::max(0,index)*stride;
    const float right=left+cardWidth;
    if(left<scroll) scroll=left;
    else if(right>scroll+viewportWidth) scroll=right-viewportWidth;
    return std::clamp(scroll,0.f,std::max(0.f,maximum));
}
inline float WheelScroll(float scroll,float maximum,float stride,float vertical,float horizontal) {
    return std::clamp(scroll-(vertical+horizontal)*stride,0.f,std::max(0.f,maximum));
}
inline StripLayout MakeStripLayout(float imageWidth,float imageHeight,std::size_t count,
 float maxWidth,float maxHeight,float paddingX,float paddingY,float frameX,float frameY,
 float gapX,float gapY,float line,float scrollbar,float border) {
    count=std::max(std::size_t{1},count);
    // Explicit child padding/content extents make sizing independent of cached
    // scrollbars and of the number of visible/virtualized cards.
    const float horizontalChrome=paddingX*2+border*2;
    const float verticalChrome=paddingY*2+border*2+line*2+gapY*2+scrollbar+frameY*2;
    const float scale=std::min({1.f,std::max(1.f,maxWidth-horizontalChrome-frameX*2)/std::max(1.f,imageWidth),std::max(1.f,maxHeight-verticalChrome)/std::max(1.f,imageHeight)});
    StripLayout l{};l.imageWidth=imageWidth*scale;l.imageHeight=imageHeight*scale;
    l.cardWidth=l.imageWidth+frameX*2;l.cardHeight=l.imageHeight+frameY*2;
    l.stride=l.cardWidth+gapX;
    l.columns=std::min(count,static_cast<std::size_t>(std::max(1.f,std::floor((maxWidth-horizontalChrome+gapX)/l.stride))));
    l.scrolling=l.columns<count;l.contentWidth=count*l.stride-gapX;
    l.contentHeight=l.cardHeight+gapY+line;
    l.width=l.scrolling?maxWidth:horizontalChrome+l.columns*l.stride-gapX;
    l.height=paddingY*2+border*2+(line+gapY)+l.contentHeight+(l.scrolling?scrollbar:0);
    return l;
}
}
