std::string KeyLabel(unsigned key) {
    if(!key) return Tr("Disabled");
    wchar_t label[128]{};
    const LONG scan=static_cast<LONG>((key&0x7F)<<16)|((key&0x80)?(1<<24):0);
    if(!GetKeyNameTextW(scan,label,128)) return std::format("Key {}",key);
    char utf8[512]{}; WideCharToMultiByte(CP_UTF8,0,label,-1,utf8,512,nullptr,nullptr); return utf8;
}
std::string PadLabel(unsigned key) {
    switch(key) {
    case 0:return "Disabled"; case 1:return "D-pad Up";case 2:return "D-pad Down";case 4:return "D-pad Left";case 8:return "D-pad Right";
    case 9:return "LT / L2";case 10:return "RT / R2";case 16:return "Start / Menu";case 32:return "Back / View";
    case 64:return "Left stick click";case 128:return "Right stick click";case 256:return "LB / L1";case 512:return "RB / R1";
    case 4096:return "A / Cross";case 8192:return "B / Circle";case 16384:return "X / Square";case 32768:return "Y / Triangle";
    default:return std::format("Button {}",key);
    }
}
unsigned HeldKeyboardModifiers() {
    return ((GetAsyncKeyState(VK_CONTROL)&0x8000)?HotkeyChord::Ctrl:0u) |
           ((GetAsyncKeyState(VK_SHIFT)&0x8000)?HotkeyChord::Shift:0u) |
           ((GetAsyncKeyState(VK_MENU)&0x8000)?HotkeyChord::Alt:0u);
}
std::string OpeningKeyLabel() {
    const auto mods=keyboardModifiers.load();std::string label;
    if(mods&HotkeyChord::Ctrl) label+="Ctrl + ";
    if(mods&HotkeyChord::Shift) label+="Shift + ";
    if(mods&HotkeyChord::Alt) label+="Alt + ";
    return label+KeyLabel(hotkey.load());
}
InputSettings CurrentInputSettings() {
    InputSettings s{hotkey.load(),gamepadHotkey.load(),padHoldSeconds.load()}; s.keyboardModifiers=keyboardModifiers.load();
    s.autoKeyboard=autoKeyboard;s.autoModifiers=autoModifiers;s.autoGamepad=autoGamepad;s.autoHoldSeconds=autoHoldSeconds;
    for(unsigned n=0;n<3;++n) {s.captureKeys[n]=captureKeys[n];s.capturePads[n]=capturePads[n];} return s;
}
void SaveInputSettings(const InputSettings& s) {
    WriteInputSettings(s,inputSettingsFile);
    autoKeyboard=s.autoKeyboard;autoModifiers=s.autoModifiers;autoGamepad=s.autoGamepad;autoHoldSeconds=s.autoHoldSeconds;
    hotkey=s.keyboard; keyboardModifiers=s.keyboardModifiers; gamepadHotkey=s.gamepad; padHoldSeconds=s.holdSeconds;
    for(unsigned n=0;n<3;++n) {captureKeys[n]=s.captureKeys[n];capturePads[n]=s.capturePads[n];}
}
#include "PadInputPolicy.h"
unsigned framePadEdges{},framePadRepeat{};
PadInputPolicy::RepeatState padRepeatState;
unsigned appliedPadReset{};

void UpdatePadFrame() {
    const auto serial=padResetSerial.load();
    if(appliedPadReset!=serial) {
        appliedPadReset=serial; padRepeatState.Reset();
        framePadEdges=framePadRepeat=0;
        return; // Do not treat the opening button as gallery navigation.
    }
    framePadEdges=padEdges.exchange(0);
    // Use one authoritative source for mapped UI buttons. Raw and translated
    // events can arrive in different frames; merging both double-counts a press.
    const std::pair<UI::ImGuiKey,unsigned> keys[]={
        {UI::ImGuiKey_GamepadDpadUp,1},{UI::ImGuiKey_GamepadDpadDown,2},
        {UI::ImGuiKey_GamepadDpadLeft,4},{UI::ImGuiKey_GamepadDpadRight,8},
        {UI::ImGuiKey_GamepadL1,256},{UI::ImGuiKey_GamepadR1,512},
        {UI::ImGuiKey_GamepadL2,0x10000},{UI::ImGuiKey_GamepadR2,0x20000},
        {UI::ImGuiKey_GamepadFaceDown,4096},{UI::ImGuiKey_GamepadFaceRight,8192},
        {UI::ImGuiKey_GamepadR3,128}};
    unsigned mapped=0, translatedEdges=0, translatedHeld=0;
    for(const auto& [key,bit]:keys) {
        mapped|=bit;
        if(UI::IsKeyPressed(key,false)) translatedEdges|=bit;
        if(UI::IsKeyDown(key)) translatedHeld|=bit;
    }
    framePadEdges=PadInputPolicy::SelectSource(framePadEdges,translatedEdges,mapped);
    const auto held=PadInputPolicy::SelectSource(padHeld.load(),translatedHeld,mapped);
    framePadRepeat=padRepeatState.Update(framePadEdges,held,GetTickCount64());
}

bool RawPadPressed(unsigned code,bool repeat=false) {
    const unsigned bit=code==9?0x10000u:code==10?0x20000u:code;
    return code && ((repeat?framePadRepeat:framePadEdges)&bit)!=0;
}
bool PadPressed(UI::ImGuiKey key,bool repeat=false) {
    unsigned code=0;
    switch(key) {
    case UI::ImGuiKey_GamepadDpadUp:code=1;break;case UI::ImGuiKey_GamepadDpadDown:code=2;break;
    case UI::ImGuiKey_GamepadDpadLeft:code=4;break;case UI::ImGuiKey_GamepadDpadRight:code=8;break;
    case UI::ImGuiKey_GamepadL1:code=256;break;case UI::ImGuiKey_GamepadR1:code=512;break;
    case UI::ImGuiKey_GamepadL2:code=9;break;case UI::ImGuiKey_GamepadR2:code=10;break;
    case UI::ImGuiKey_GamepadFaceDown:code=4096;break;case UI::ImGuiKey_GamepadFaceRight:code=8192;break;
    case UI::ImGuiKey_GamepadR3:code=128;break;
    default:return UI::IsKeyPressed(key,repeat);
    } return RawPadPressed(code,repeat);
}
std::string CaptureHint(unsigned n) {
    return KeyLabel(captureKeys[n])+" / "+PadLabel(capturePads[n]);
}
void ApplyHotkeys(unsigned keyboard,unsigned pad,float seconds,unsigned modifiers=8) {
    try {

        auto s=CurrentInputSettings(); s.keyboard=keyboard;s.gamepad=pad;s.holdSeconds=seconds; if(modifiers<=7) s.keyboardModifiers=modifiers; SaveInputSettings(s);
        hotkey=keyboard; gamepadHotkey=pad; padHoldSeconds=seconds;
        SetStatus("Menu hotkeys saved.");
    } catch(const std::exception& e) {SetStatus(e.what());}
}
void ProcessCapturedHotkey() {
    const auto result=capturedBinding.exchange(0);
    if(result) {
        if(HotkeyChord::Mode(result)==1) ApplyHotkeys(result&0xFFFF,gamepadHotkey.load(),padHoldSeconds.load(),HotkeyChord::Modifiers(result));
        else if(HotkeyChord::Mode(result)==2) ApplyHotkeys(hotkey.load(),result&0xFFFF,padHoldSeconds.load());
        else if(HotkeyChord::Mode(result)==9 || HotkeyChord::Mode(result)==10) {
            try {auto s=CurrentInputSettings();if(HotkeyChord::Mode(result)==9) {s.autoKeyboard=result&0xFFFF;s.autoModifiers=HotkeyChord::Modifiers(result);}else s.autoGamepad=result&0xFFFF;SaveInputSettings(s);SetStatus("Menu hotkeys saved.");}
            catch(const std::exception& e){SetStatus(e.what());}
        }
        else {try {auto s=CurrentInputSettings();const unsigned mode=HotkeyChord::Mode(result),n=(mode-3)/2;
            if(n<3) {if(mode%2) s.captureKeys[n]=result&0xFFFF;else s.capturePads[n]=result&0xFFFF; SaveInputSettings(s);}
        }catch(const std::exception& e){SetStatus(e.what());}}
    }
    if(bindingMode && GetTickCount64()>bindingDeadline) bindingMode=0;
}
void RenderHotkeys() {
    UI::Text(Tr("Open menu (keyboard): %s"),OpeningKeyLabel().c_str());
    if(UI::Button(Label("Change keyboard key"))) {bindingMode=1;bindingDeadline=GetTickCount64()+10000;}
    if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Supports Ctrl, Shift and Alt combinations. Bind a key alone to remove modifiers. Other mods may still react to the same key."));
    UI::Text(Tr("Open menu (gamepad): %s / hold %.1f s"),Tr(PadLabel(gamepadHotkey.load()).c_str()),padHoldSeconds.load());
    if(UI::Button(Label("Change gamepad button"))) {bindingMode=2;bindingDeadline=GetTickCount64()+10000;}
    UI::SameLine(); if(UI::Button(Label("Disable gamepad hotkey"))) ApplyHotkeys(hotkey.load(),0,padHoldSeconds.load());
    float seconds=padHoldSeconds.load();
    if(UI::SliderFloat(Label("Hold duration"),&seconds,.4f,3.f,"%.1f s")) padHoldSeconds=seconds;
    if(UI::IsItemDeactivatedAfterEdit()) ApplyHotkeys(hotkey.load(),gamepadHotkey.load(),padHoldSeconds.load());
    {
        UI::Separator();
        const char* names[]{"Save outfit","Save headgear","Save accessories"};
        for(unsigned n=0;n<3;++n) {
            UI::Text("%s: %s",Tr(names[n]),CaptureHint(n).c_str());
            if(UI::Button((std::string(Tr("Keyboard"))+"##capture"+std::to_string(n)).c_str())) {bindingMode=3+n*2;bindingDeadline=GetTickCount64()+10000;}
            UI::SameLine();
            if(UI::Button((std::string(Tr("Gamepad"))+"##capture"+std::to_string(n)).c_str())) {bindingMode=4+n*2;bindingDeadline=GetTickCount64()+10000;}
            UI::SameLine();
            if(UI::Button((std::string(Tr("Disable"))+"##capture"+std::to_string(n)).c_str())) {try {auto s=CurrentInputSettings();s.captureKeys[n]=0;s.capturePads[n]=0;SaveInputSettings(s);}catch(const std::exception& e){SetStatus(e.what());}}
        }
        if(UI::Button(Label("Reset capture keys"))) {try {auto s=CurrentInputSettings();s.captureKeys={67,68,87};s.capturePads={32768,16384,64};SaveInputSettings(s);}catch(const std::exception& e){SetStatus(e.what());}}
        UI::TextWrapped(Tr("Navigation buttons are reserved. Disable a conflicting capture binding before assigning it."));
    }
    if(bindingMode) {
        UI::TextUnformatted(Tr((bindingMode==1 || bindingMode==9)?"Hold Ctrl, Shift or Alt and press a key, or press a key alone. Escape cancels.":"Press the new key/button. Escape or Cancel stops capture."));
        if(UI::Button(Label("Cancel binding"))) bindingMode=0;
    }
    UI::TextWrapped(Tr("Short gamepad presses pass through. Another mod may also react during a long press; choose an unused button if needed."));
    UI::Separator();
}

void RenderAutoHotkeys() {
    UI::PushID("autoHotkeys");
    std::string key;
    const unsigned mods=autoModifiers.load();
    if(mods&HotkeyChord::Ctrl) key+="Ctrl + ";
    if(mods&HotkeyChord::Shift) key+="Shift + ";
    if(mods&HotkeyChord::Alt) key+="Alt + ";
    key+=KeyLabel(autoKeyboard.load());
    UI::TextWrapped("%s",Tr("Toggle automatic outfit changes ON/OFF. A paused system resumes when switched ON."));
    UI::Text(Tr("Toggle auto outfits (keyboard): %s"),key.c_str());
    if(UI::Button(Label("Change keyboard key"))) {bindingMode=9;bindingDeadline=GetTickCount64()+10000;}
    if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Supports Ctrl, Shift and Alt combinations. Bind a key alone to remove modifiers. Other mods may still react to the same key."));
    UI::SameLine();
    if(UI::Button(Label("Disable keyboard hotkey"))) {try{auto s=CurrentInputSettings();s.autoKeyboard=0;s.autoModifiers=0;SaveInputSettings(s);}catch(const std::exception& e){SetStatus(e.what());}}
    UI::Text(Tr("Toggle auto outfits (gamepad): %s / hold %.1f s"),Tr(PadLabel(autoGamepad.load()).c_str()),autoHoldSeconds.load());
    if(UI::Button(Label("Change gamepad button"))) {bindingMode=10;bindingDeadline=GetTickCount64()+10000;}
    UI::SameLine();
    if(UI::Button(Label("Disable gamepad hotkey"))) {try{auto s=CurrentInputSettings();s.autoGamepad=0;SaveInputSettings(s);}catch(const std::exception& e){SetStatus(e.what());}}
    float seconds=autoHoldSeconds.load();
    if(UI::SliderFloat(Label("Hold duration"),&seconds,.4f,3.f,"%.1f s")) autoHoldSeconds=seconds;
    if(UI::IsItemDeactivatedAfterEdit()) {try{SaveInputSettings(CurrentInputSettings());}catch(const std::exception& e){SetStatus(e.what());}}
    if(bindingMode==9 || bindingMode==10) {
        UI::TextWrapped("%s",Tr(bindingMode==9?"Hold Ctrl, Shift or Alt and press a key, or press a key alone. Escape cancels.":"Press the new key/button. Escape or Cancel stops capture."));
        if(UI::Button(Label("Cancel binding"))) bindingMode=0;
    }
    UI::TextWrapped("%s",Tr("Short gamepad presses pass through. Another mod may also react during a long press; choose an unused button if needed."));
    UI::PopID();
}
