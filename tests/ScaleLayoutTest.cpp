#include "ScaleLayout.h"
#include "GalleryLayout.h"
#include <cmath>
#include <iostream>
#include <limits>
int main() {
    using namespace Gallery;
    const SurfaceSize display{3840,2160};
    LayoutSurface s{display,{2560,1440},{2560,1440},true};
    int failures=0;
    auto check=[&](SurfaceSize fb,SurfaceSize expected,const char* name){
        const auto r=ResolveLayoutSize(display,fb,s);
        if(r.width!=expected.width || r.height!=expected.height){std::cerr<<name<<'\n'; ++failures;}
    };
    check({1,1},{2560,1440},"reported DLSS ON");
    s.buffer=s.viewport=display; check({1,1},display,"reported DLSS OFF");
    s.buffer=s.viewport={2560,1440}; check({1.5f,1.5f},display,"existing framebuffer compensation");
    s.valid=false; check({1,1},display,"missing surface"); s.valid=true;
    s.viewport=display; check({1,1},display,"different viewport");
    s.viewport=s.buffer={2560,1080}; check({1,1},display,"different aspect ratio");
    s.viewport=s.buffer={5120,2880}; check({1,1},display,"supersampling");
    s.viewport=s.buffer={0,0}; check({1,1},display,"minimized surface");
    s.viewport=s.buffer={std::numeric_limits<float>::quiet_NaN(),1440}; check({1,1},display,"invalid surface");
    s.viewport=s.buffer={2560,1440}; s.swap={1920,1080}; check({1,1},display,"unmatched swap dimensions");
    for(int columns=2;columns<=6;++columns) for(bool portrait:{false,true}) {
        const auto fhd=MakeThumbnailLayout(950,1920,1080,columns,portrait);
        const auto tall=MakeThumbnailLayout(950,1920,1200,columns,portrait);
        const auto qhd=MakeThumbnailLayout(950,2560,1440,columns,portrait);
        const auto wide=MakeThumbnailLayout(950,2560,1080,columns,portrait);
        if(fhd.width!=tall.width || fhd.height!=tall.height || fhd.uvLeft!=tall.uvLeft || fhd.uvRight!=tall.uvRight || std::abs(fhd.height-qhd.height)>.001f || wide.height>=fhd.height) {
            std::cerr<<"Thumbnail aspect policy failed\n"; ++failures;
        }
    }
    return failures?1:0;
}
