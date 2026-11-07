#include "UiScale.h"
#include <cassert>
#include <stdexcept>
// Exercise the production lifetime guard with a small SMF API double.
// Runtime font rendering still requires the game/SMF integration test.
namespace UI {
struct ImVec2 {float x{},y{};};
struct ImGuiStyle {float padding=8, border=1;};
struct IO {float FontGlobalScale=1.25f;};
inline ImGuiStyle style;
inline IO io;
inline int font, depth;
inline float cachedFont=30;
inline auto GetStyle(){return &style;}
inline auto GetIO(){return &io;}
inline auto GetFont(){return &font;}
namespace ImGuiStyleManager {void ScaleAllSizes(ImGuiStyle* s,float f){s->padding*=f;s->border*=f;}}
inline void PushFont(int*){++depth;cachedFont=24*io.FontGlobalScale;}
inline void PopFont(){--depth;cachedFont=24*io.FontGlobalScale;}
inline constexpr int ImGuiCond_Appearing=1;
inline void SetNextWindowSizeConstraints(ImVec2,ImVec2){}
inline void SetNextWindowPos(ImVec2,int,ImVec2){}
}
namespace Gallery {
#include "UiScaleScope.inc"
}
int main(){
    using namespace Gallery;
    for(int i=0;i<10;++i){
        try {
            ScopedGalleryScale outer({1280,720},1);
            assert(UI::cachedFont==15 && UI::style.padding==4);
            {
                ScopedGalleryScale inner({2560,1440},1.5f);
                assert(UI::cachedFont==22.5f && UI::style.padding==6);
            }
            assert(UI::cachedFont==15 && uiFrameScale==.5f);
            throw std::runtime_error("early exit");
        }catch(const std::runtime_error&){}
        assert(UI::style.padding==8 && UI::style.border==1);
        assert(UI::io.FontGlobalScale==1.25f && UI::cachedFont==30);
        assert(UI::depth==0 && uiFrameScale==1 && uiLayoutScreen.x==0);
    }
}
