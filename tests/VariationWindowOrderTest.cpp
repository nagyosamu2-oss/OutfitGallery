#include "PointerPositionScope.h"
// CPU-only integration against the local SMF reference's Dear ImGui core.
// No game, DX11 hooks, or SMF DLL are loaded by this test.
#include "imgui.h"
#include "imgui_internal.h"
#include "VariationStripLayout.h"
#include <stdexcept>
#include <iostream>
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int Index(ImGuiWindow* window){for(int i=0;i<GImGui->Windows.Size;++i)if(GImGui->Windows[i]==window)return i;return -1;}
bool Frame(bool show,bool raise,bool galleryFocus) {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({900,900});
    ImGui::Begin("Gallery",nullptr,ImGuiWindowFlags_NoSavedSettings);
    auto* gallery=ImGui::GetCurrentWindow();
    if(galleryFocus)ImGui::FocusWindow(gallery);
    ImGui::BeginChild("Contents");ImGui::Button("Photo",{150,240});
    ImGuiWindow* strip=nullptr;
    if(show) {
        ImGui::SetNextWindowPos({100,150});ImGui::SetNextWindowSize({700,330});
        const bool rendered=ImGui::Begin("##VariationStrip",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoNavInputs);
        Check(rendered,"strip must render");strip=ImGui::GetCurrentWindow();
        auto* navBefore=GImGui->NavWindow;
        if(raise)ImGui::BringWindowToDisplayFront(strip);
        Check(GImGui->NavWindow==navBefore,"raising display order must not steal input focus");
        ImGui::BeginChild("Choices",{0,0},0,ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Button("Variant",{150,240});ImGui::EndChild();ImGui::End();
    }
    ImGui::EndChild();ImGui::End();ImGui::Render();
    return strip && Index(strip)>Index(gallery);
}
void LayoutFrame(int count,float imageWidth,float imageHeight,float scale) {
    ImGui::NewFrame();auto& style=ImGui::GetStyle();
    const float fx=3*scale,fy=3*scale;
    const auto layout=Gallery::Variations::MakeStripLayout(imageWidth,imageHeight,count,900,850,
        style.WindowPadding.x,style.WindowPadding.y,fx,fy,style.ItemSpacing.x,style.ItemSpacing.y,ImGui::GetTextLineHeight(),style.ScrollbarSize,style.WindowBorderSize);
    Check(!layout.scrolling || layout.width==900.f,"overflow expands to all available width");
    Check(layout.imageWidth==imageWidth && layout.imageHeight==imageHeight,"normal viewport preserves thumbnail dimensions");
    ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({layout.width,layout.height});
    ImGui::Begin("Layout strip",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Variations");
    ImGui::SetNextWindowContentSize({layout.contentWidth,layout.contentHeight});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::BeginChild("Layout photos",{0,0},0,ImGuiWindowFlags_HorizontalScrollbar);
    auto* child=ImGui::GetCurrentWindow();const auto origin=ImGui::GetCursorPos();
    ImGui::SetScrollX(0);
    for(int i=0;i<count;++i) {
        ImGui::SetCursorPos({origin.x+i*layout.stride,origin.y});ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{fx,fy});ImGui::PushID(i);
        ImGui::ImageButton("Photo",nullptr,{layout.imageWidth,layout.imageHeight});ImGui::PopID();ImGui::PopStyleVar();
        if(i==static_cast<int>(layout.columns)-1)Check(ImGui::GetItemRectMax().x<=child->InnerClipRect.Max.x+.01f,"last whole card must fit in clip rectangle");
        ImGui::Text("%d / %d",i+1,count);ImGui::EndGroup();
    }
    Check(!child->ScrollbarY,"caption/footer height must avoid vertical scrollbar");
    Check(child->ScrollbarX==layout.scrolling,"horizontal scrollbar only for overflow");
    ImGui::EndChild();ImGui::PopStyleVar();ImGui::End();ImGui::Render();
}
void WheelFrames() {
    float scroll=0;
    auto frame=[&](float wheel,bool hovered) {
        auto& io=ImGui::GetIO();io.AddMousePosEvent(hovered?100.f:1100.f,hovered?160.f:900.f);io.AddMouseWheelEvent(0,wheel);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({900,850});
        ImGui::Begin("Wheel gallery",nullptr,ImGuiWindowFlags_NoSavedSettings);
        ImGui::Dummy({100,1800});auto* gallery=ImGui::GetCurrentWindow();
        ImGui::SetNextWindowPos({40,100});ImGui::SetNextWindowSize({600,400});
        ImGui::Begin("Wheel strip",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoScrollbar);
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());ImGui::TextUnformatted("Variations");
        ImGui::SetNextWindowContentSize({1800,250});
        ImGui::BeginChild("Wheel photos",{0,0},0,ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && io.MouseWheel!=0)
            ImGui::SetScrollX(Gallery::Variations::WheelScroll(ImGui::GetScrollX(),ImGui::GetScrollMaxX(),200,io.MouseWheel,io.MouseWheelH));
        scroll=ImGui::GetScrollX();ImGui::Dummy({1800,250});
        ImGui::EndChild();ImGui::End();
        Check(gallery->Scroll.y==0 && gallery->ScrollTarget.y==FLT_MAX,"wheel over strip must not scroll underlying gallery");
        ImGui::End();ImGui::Render();
    };
    frame(0,true);frame(0,true);frame(-1,true);frame(0,true);
    Check(scroll==200,"wheel down scrolls one card right without default double scroll");
    frame(1,true);frame(0,true);Check(scroll==0,"wheel up scrolls left");
    frame(-1,false);frame(0,false);Check(scroll==0,"wheel outside strip leaves strip unchanged");
    Check(Gallery::Variations::WheelScroll(0,1000,200,1,0)==0,"left edge clamps");
    Check(Gallery::Variations::WheelScroll(900,1000,200,-1,0)==1000,"right edge clamps");
}
void SourceSelectionFrames() {
    using Gallery::Variations::OrganizerStripArea;
    auto below=OrganizerStripArea(300,320,50,1000,32,6);
    Check(below.top==332,"strip exposes source selection band below");
    auto above=OrganizerStripArea(850,320,50,1000,32,6);
    Check(above.top+320<=844,"bottom-edge strip moves above source");
    for(float anchor:{50.f,300.f,500.f,850.f,980.f}) {
        auto area=OrganizerStripArea(anchor,800,50,1000,32,6);
        Check(area.top>=50 && area.top+area.height<=1000,"placement remains in viewport");
        Check(area.top>=anchor+32 || area.top+area.height<=anchor-6,"selection band remains exposed even with constrained height");
    }
    bool selected=true,clicked=false;
    for(int step=0;step<6;++step) {
        auto& io=ImGui::GetIO();io.AddMousePosEvent(250,310);
        if(step==3)io.AddMouseButtonEvent(0,true);
        if(step==4)io.AddMouseButtonEvent(0,false);
        ImGui::NewFrame();ImGui::Begin("Selection host");
        if(step==0)ImGui::OpenPopup("Selection organizer");
        ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({900,900});
        Check(ImGui::BeginPopupModal("Selection organizer",nullptr,ImGuiWindowFlags_NoSavedSettings),"organizer modal opens");
        ImGui::SetCursorScreenPos({200,300});
        const bool button=ImGui::Button("Representative",{160,240});
        const bool sourceSelectionClick=!ImGui::IsItemHovered() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseClicked(0);
        if(button || sourceSelectionClick) {selected=!selected;clicked=true;}
        if(step==1)ImGui::OpenPopup("Selection variations");
        ImGui::SetNextWindowPos({150,332});ImGui::SetNextWindowSize({400,320});
        if(ImGui::BeginPopup("Selection variations",ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextUnformatted("Choices");ImGui::EndPopup();
        }
        if(step==5)ImGui::CloseCurrentPopup();
        ImGui::EndPopup();ImGui::End();ImGui::Render();
    }
    Check(clicked && !selected,"exposed source deselects on click while variation popup is open");
}
void RepresentativeDragFrames() {
    // Real mouse frames: an exposed parent card is blocked by its variation popup.
    // Native source cannot start there; the public external source delivers to normal targets.
    bool armed=false,dragging=false,delivered=false,blocked=false,closed=false;
    for(int step=0;step<10;++step) {
        auto& io=ImGui::GetIO();
        io.AddMousePosEvent(step>=6?730.f:(step>=4?290.f:250.f),310.f);
        if(step==3)io.AddMouseButtonEvent(0,true);
        if(step==8)io.AddMouseButtonEvent(0,false);
        ImGui::NewFrame();ImGui::Begin("Drag host");
        if(step==0)ImGui::OpenPopup("Drag organizer");
        ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({1000,900});
        Check(ImGui::BeginPopupModal("Drag organizer",nullptr,ImGuiWindowFlags_NoSavedSettings),"drag modal opens");
        ImGui::SetCursorScreenPos({200,300});ImGui::Button("Drag representative",{160,240});
        const bool sourceClick=!ImGui::IsItemHovered() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseClicked(0);
        if(sourceClick){armed=true;blocked=true;}
        ImGui::SetCursorScreenPos({700,300});ImGui::Button("Drop destination",{160,240});
        if(ImGui::BeginDragDropTarget()) {
            if(const auto* p=ImGui::AcceptDragDropPayload("OUTFIT_ORGANIZE",ImGuiDragDropFlags_AcceptBeforeDelivery))
                if(p->Delivery)delivered=true;
            ImGui::EndDragDropTarget();
        }
        if(armed) {
            if(ImGui::IsMouseDown(0) && ImGui::IsMouseDragging(0,24.f))dragging=true;
            if(dragging && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
                const int token=1;ImGui::SetDragDropPayload("OUTFIT_ORGANIZE",&token,sizeof(token));
                ImGui::TextUnformatted("Whole group");ImGui::EndDragDropSource();
            }
            if(!ImGui::IsMouseDown(0)){armed=false;dragging=false;}
        }
        if(step==1)ImGui::OpenPopup("Drag variations");
        ImGui::SetNextWindowPos({150,332});ImGui::SetNextWindowSize({400,320});
        if(ImGui::BeginPopup("Drag variations",ImGuiWindowFlags_NoSavedSettings)) {
            if(ImGui::GetDragDropPayload()){ImGui::CloseCurrentPopup();closed=true;}
            ImGui::TextUnformatted("Choices");ImGui::EndPopup();
        }
        if(ImGui::GetDragDropPayload() && !ImGui::IsPopupOpen("Drag variations"))closed=true;
        if(step==9)ImGui::CloseCurrentPopup();
        ImGui::EndPopup();ImGui::End();ImGui::Render();
    }
    Check(blocked,"gesture starts from popup-blocked exposed representative");
    Check(closed,"drag closes variation popup");
    Check(delivered,"external representative drag delivers to existing reorder target");
    Check(!armed && !dragging,"release clears representative gesture");
}
void PadRevealFrames() {
    for(int index:{0,1,5,6,7,11,12,0,12,11,5,1,0}) {
        // SetScrollX applies on the following frame, as in the actual strip.
        for(int settle=0;settle<2;++settle) {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({800,400});
            ImGui::Begin("Pad reveal",nullptr,ImGuiWindowFlags_NoSavedSettings);
            ImGui::SetNextWindowContentSize({13*206.f-6.f,260});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
            ImGui::BeginChild("Pad photos",{0,0},0,ImGuiWindowFlags_HorizontalScrollbar);
            auto* child=ImGui::GetCurrentWindow();const auto origin=ImGui::GetCursorPos();
            const float available=ImGui::GetWindowWidth()-ImGui::GetStyle().WindowBorderSize*2;
            auto target=Gallery::Variations::RevealScroll(ImGui::GetScrollX(),ImGui::GetScrollMaxX(),index,206,200,available);
            ImGui::SetScrollX(target);
            ImGui::SetCursorPos({origin.x+index*206,origin.y});ImGui::Button("Selected photo",{200,240});
            if(settle==1) {
                Check(ImGui::GetItemRectMin().x>=child->InnerClipRect.Min.x-.1f,"pad selected photo left edge visible");
                Check(ImGui::GetItemRectMax().x<=child->InnerClipRect.Max.x+.1f,"pad selected photo right edge visible");
            }
            ImGui::EndChild();ImGui::PopStyleVar();ImGui::End();ImGui::Render();
        }
    }
}
void ContextFrame() {
    // Model real user timing: parent visible, right-click, then click an action.
    for(int step=0;step<3;++step) {
        ImGui::NewFrame();ImGui::Begin("Organizer test");if(step==0)ImGui::OpenPopup("Variations");
        Check(ImGui::BeginPopup("Variations"),"variation popup opens");
        ImGui::TextUnformatted("Photos");
        if(step==1)ImGui::OpenPopup("Actions");
        if(step>=1) {
            Check(ImGui::BeginPopup("Actions"),"nested context opens");
            ImGui::TextUnformatted("Actions");if(step==2)ImGui::CloseCurrentPopup();ImGui::EndPopup();
        }
        ImGui::EndPopup();Check(ImGui::IsPopupOpen("Variations"),"context preserves variation parent");
        if(step==2 && ImGui::BeginPopup("Variations")){ImGui::CloseCurrentPopup();ImGui::EndPopup();}
        ImGui::End();ImGui::Render();
    }
}
void PointerHoverFrames() {
    auto& io=ImGui::GetIO();
    for(int frame=0;frame<4;++frame) {
        io.AddMousePosEvent(60,60);ImGui::NewFrame();
        ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({300,240});
        ImGui::Begin("Pointer regression",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
        const auto pos=io.MousePos,delta=io.MouseDelta;
        const bool hidden=frame==2;
        {
            PointerPositionScope scope(io,hidden);
            ImGui::Button("Photo",{150,120});
            if(frame>=1) Check(ImGui::IsItemHovered()==!hidden,"stationary cursor hovers in mouse mode only");
            if(hidden) Check(io.MouseDelta.x==0 && io.MouseDelta.y==0,"pad camera sees no stale mouse delta");
        }
        Check(io.MousePos.x==pos.x && io.MousePos.y==pos.y && io.MouseDelta.x==delta.x && io.MouseDelta.y==delta.y,"shared pointer IO restored");
        ImGui::End();ImGui::Render();
    }
}
int main(){try {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1920,1080};io.DeltaTime=1.f/60;
    unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    Check(Frame(true,true,false),"initial strip on top");
    Check(!Frame(true,false,true),"negative control: gallery focus hides an unraised persistent strip");
    Check(Frame(true,true,true),"raising restores visible strip after gallery focus");
    for(int repeat=0;repeat<10;++repeat) {
        Frame(false,false,true);
        Check(!Frame(true,false,true),"negative control: reopen alone does not repair z order");
        Check(Frame(true,true,true),"repeated close, focus and reopen stays visible with fix");
    }
    for(int count:{2,3,6,20})for(float scale:{.75f,1.f,1.5f}) {
        LayoutFrame(count,160*scale,240*scale,scale);
        LayoutFrame(count,300*scale,300*scale,scale);
    }
    PointerHoverFrames();
    WheelFrames();
    PadRevealFrames();
    ContextFrame();
    SourceSelectionFrames();
    RepresentativeDragFrames();
    ImGui::DestroyContext();std::cout<<"Window stacking, exact strip layout and nested context tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what();if(ImGui::GetCurrentContext())ImGui::DestroyContext();return 1;}}
