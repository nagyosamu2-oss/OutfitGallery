// Only active while SMF invokes RenderStudio on its UI thread.
float uiFrameScale=1.f;
UI::ImVec2 uiLayoutScreen{};
float UiPx(float pixels) { return pixels*uiFrameScale; }
class ScopedGalleryScale {
    UI::ImGuiStyle previousStyle;
    float previousFontScale, previousFrameScale;
    UI::ImVec2 previousScreen;
public:
    ScopedGalleryScale(UI::ImVec2 screen,float zoom) :
        previousStyle(*UI::GetStyle()), previousFontScale(UI::GetIO()->FontGlobalScale),
        previousFrameScale(uiFrameScale), previousScreen(uiLayoutScreen) {
        uiLayoutScreen=screen;
        uiFrameScale=ResolveUiScale(screen.x,screen.y,zoom);
        UI::ImGuiStyleManager::ScaleAllSizes(UI::GetStyle(),uiFrameScale);
        UI::GetIO()->FontGlobalScale=previousFontScale*uiFrameScale;
        // Refresh the cached font size as well as IO. Pop after restoring IO.
        UI::PushFont(UI::GetFont());
    }
    ~ScopedGalleryScale() {
        *UI::GetStyle()=previousStyle;
        UI::GetIO()->FontGlobalScale=previousFontScale;
        UI::PopFont();
        uiFrameScale=previousFrameScale;uiLayoutScreen=previousScreen;
    }
    ScopedGalleryScale(const ScopedGalleryScale&)=delete;
    ScopedGalleryScale& operator=(const ScopedGalleryScale&)=delete;
};
void PrepareGalleryPopup() {
    UI::SetNextWindowSizeConstraints({0,0},{uiLayoutScreen.x*.94f,uiLayoutScreen.y*.94f});
    UI::SetNextWindowPos({uiLayoutScreen.x*.5f,uiLayoutScreen.y*.5f},UI::ImGuiCond_Appearing,{.5f,.5f});
}
