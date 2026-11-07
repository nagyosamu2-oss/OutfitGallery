#include "UiScale.h"
#include "GalleryLayout.h"
#include <cassert>
#include <limits>
int main() {
    using namespace Gallery;
    assert(ResolveUiScale(2560,1440,1)==1);
    assert(ResolveUiScale(1920,1080,1)==.75f);
    assert(ResolveUiScale(1280,720,1)==.5f);
    assert(ResolveUiScale(3840,2160,1)==1.5f);
    assert(ResolveUiScale(3440,1440,1)==1);
    assert(ResolveUiScale(1280,720,1.5f)==.75f);
    assert(ResolveUiScale(0,0,1)==1);
    assert(NormalizeUiZoom(std::numeric_limits<float>::quiet_NaN())==1);
    // Upscaled 720->1440: coordinate-sized controls preserve the WQHD ratio.
    assert(24*ResolveUiScale(1280,720,1)*2==24*ResolveUiScale(2560,1440,1));
    for(int cols=2;cols<=6;++cols) for(bool portrait:{false,true}) {
        auto a=MakeThumbnailLayout(1200,2560,1440,cols,portrait,1);
        auto b=MakeThumbnailLayout(600,1280,720,cols,portrait,.5f);
        assert(std::abs(a.width-b.width*2)<.001f);
        assert(std::abs(a.height-b.height*2)<.001f);
    }
}
