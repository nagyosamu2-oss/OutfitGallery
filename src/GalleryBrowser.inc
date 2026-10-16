// Included inside Gallery; UI-owned state is only changed on the render thread.
Library library;
bool libraryWritable=true;
unsigned libraryRevision{};
MissingPresetCounts missingCounts;
void UpdateMissingCounts() {
    static unsigned lastLibrary=~0u,lastPresets=~0u,lastResult=~0u;
    if(lastLibrary==libraryRevision && lastPresets==presetRevision.load() && lastResult==missingOrderSerial) return;
    std::scoped_lock lock(presetsMutex);
    missingCounts=CountMissingPresets(presets,missingResults,[](const std::string& photo) {
        const auto it=library.labels.find(photo); return it!=library.labels.end() && it->second.deleted;
    });
    lastLibrary=libraryRevision; lastPresets=presetRevision.load(); lastResult=missingOrderSerial;
}

const std::filesystem::path libraryFile="Data/SKSE/Plugins/OutfitGallery/Library.json";

bool CommitLibrary(Library next) {
    if(!libraryWritable) { SetStatus("Library could not be read. Restore Library.json before editing."); return false; }
    try { WriteLibrary(next,libraryFile); library=std::move(next); ++libraryRevision; return true; }
    catch(const std::exception& e) { SetStatus(e.what()); return false; }
}

#include "GalleryOrganizeUI.inc"

void RenderBrowser(bool& addMissing) {
    static char search[160]{}, editName[241]{}, tabName[241]{};
    static std::string category, selected, contextPhoto, editCategory;
    static bool newCategory{};
    static std::vector<std::string> deletePhotos;
    bool openDelete=false, openScope=false;
    std::string moveMissingToTrash;
    static Preset scopeEditing;
    static std::uint32_t scopeDraft{};
    static std::vector<std::string> scopeSlotless;
    static bool scopeDraftInitialized{};
    static std::optional<std::vector<std::string>> scopeItems;

    static std::vector<std::string> loaded;
    static std::string oldFilter;
    static bool tabsFocused{}, columnsFocused{};
    static unsigned lastOpen{};
    if(lastOpen!=studioOpenSerial.load()) {
        EndOrganizer();
        SetMissingCheckMode(false);
        lastOpen=studioOpenSerial.load(); tabsFocused=false; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false; selected.clear(); oldFilter.clear(); search[0]=0;
        category=ResolveStartupTab(startupTab,library);
        UI::ClearActiveID();
    }
    if(organizer.active) {RenderOrganizer(); return;}
    std::optional<Preset> reveal;
    {std::scoped_lock lock(presetsMutex); reveal=std::move(revealAfterSave); revealAfterSave.reset();}
    if(reveal) {
        category=reveal->accessories?"#accessories":reveal->slotMask?"#head":"";
        selected=reveal->photo; search[0]=0; oldFilter.clear(); tabsFocused=false; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false;
        UI::ClearActiveID();
    }
    bool openEdit=false, openTab=false;
    if(!photoRetakeActive && UI::CollapsingHeader(Label("Options"))) {
        if(UI::BeginCombo("Language / 言語",language==2?"External (Translation.json)":language==1?"日本語":"English")) {
            if(UI::Selectable("日本語",language==1)) {language=1; settingsChangedAt=GetTickCount64();}
            if(UI::Selectable("English",language==0)) {language=0; settingsChangedAt=GetTickCount64();}
            if(UI::Selectable("External (Translation.json)",language==2)) {language=2; settingsChangedAt=GetTickCount64();}
            UI::EndCombo();
        }
        if(UI::CollapsingHeader(Label("Capture keys"))) { RenderHotkeys(); }
        bool combatEntry=allowCombatGallery.load();
        if(UI::Checkbox(Label("Allow opening Gallery during combat (experimental)"),&combatEntry)) {
            allowCombatGallery=combatEntry;settingsChangedAt=GetTickCount64();
            SKSE::log::info("OG-STUDIO combat entry option={}",combatEntry);
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Player only. Combat may continue while Gallery is open. Killmoves, stagger and target lock remain blocked."));
        bool freeEntry=allowFreeCamera.load();
        if(UI::Checkbox(Label("Allow opening from free camera (experimental)"),&freeEntry)) {allowFreeCamera=freeEntry; settingsChangedAt=GetTickCount64();}
        UI::Separator();
        bool preview=detachedPreview.load();
        UI::BeginDisabled(saveBusy);
        if(UI::Checkbox(Label("Independent preview"),&preview)) {
            detachedPreview=preview; previewSettled=0; resetPreviewRect=true;
            settingsChangedAt=GetTickCount64();
        }
        if(preview) {
            UI::TextWrapped(Tr("Drag the preview title to move it; drag its corner to resize. Photos use the centered view. Effects depend on your graphics setup."));
            if(UI::Button(Label("Reset preview position"))) {previewRect={.54f,.08f,.45f,.84f}; resetPreviewRect=true; settingsChangedAt=GetTickCount64();}
        }
        UI::EndDisabled();
        if(UI::Checkbox(Label("Compact display"),&compactGallery)) {
            search[0]=0; UI::ClearActiveID(); SaveSettingsIfDue(true);
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Hide the name and search fields. New outfits use automatic names. Turn off to enter names or search."));
        bool changed=UI::Checkbox(Label("Show preset names"),&showNames);
        changed=UI::Checkbox(Label("Show item counts"),&showCounts)||changed;
        if(changed) settingsChangedAt=GetTickCount64();
        if(UI::Checkbox(Label("Add missing base items (new game)"),&addMissing)) { SaveSettingsIfDue(true); }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Generated items use base properties; custom enchantments are not recreated."));
        bool prefer=preferEnchanted.load();
        if(UI::Checkbox(Label("Prefer registered enchanted items"),&prefer)) {preferEnchanted=prefer; SaveSettingsIfDue(true);}
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Player only. Uses owned registered enchanted items; otherwise uses normal equipment and notifies you. Applies on the next outfit change. Re-register older sets to record enchantments."));
        UI::Separator();
        if(UI::SliderInt(Label("Normal-view columns"),&gridColumns,2,6)) settingsChangedAt=GetTickCount64();
        std::vector<Category> openingTabs{{"","All"}};
        openingTabs.insert(openingTabs.end(),library.categories.begin(),library.categories.end());
        openingTabs.push_back({"#head","Head equipment"});
        openingTabs.push_back({"#accessories","Accessories"});
        openingTabs.push_back({"#trash","Trash"});
        const auto resolvedOpening=ResolveStartupTab(startupTab,library);
        const auto opening=std::find_if(openingTabs.begin(),openingTabs.end(),[&](const auto& c){return c.id==resolvedOpening;});
        const auto openingTitle=opening==openingTabs.end()?std::string(Tr("All")):CategoryTitle(*opening);
        if(UI::BeginCombo(Label("Opening tab"),openingTitle.c_str())) {
            for(const auto& c:openingTabs) {
                if(UI::Selectable((CategoryTitle(c)+"##opening"+c.id).c_str(),c.id==resolvedOpening)) {
                    startupTab=c.id; SaveSettingsIfDue(true);
                }
            }
            UI::EndCombo();
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Choose the tab shown when opening the gallery. Registration still opens the matching tab."));
        if(UI::Button(Label("+ Tab"))) {newCategory=true; tabName[0]=0; openTab=true;}
        UI::SameLine();
        UI::BeginDisabled(category.empty() || category.starts_with("#"));
        if(UI::Button(Label("Edit tab"))) {
            auto it=std::find_if(library.categories.begin(),library.categories.end(),[&](const auto& c){return c.id==category;});
            if(it!=library.categories.end()) {newCategory=false; editCategory=category; strcpy_s(tabName,it->name.c_str()); openTab=true;}
        }
        UI::EndDisabled();
        if(UI::CollapsingHeader(Label("Head equipment slots"))) {
            auto mask=partialSlots.load();
            const unsigned slots[]{30,31,41,42};
            for(auto slot:slots) {
                if(slot!=30) UI::SameLine();
                bool checked=(mask&(1u<<(slot-30)))!=0;
                if(UI::Checkbox(std::to_string(slot).c_str(),&checked)) {
                    if(checked) mask|=1u<<(slot-30); else mask&=~(1u<<(slot-30));
                    partialSlots=mask; SaveSettingsIfDue(true);
                }
            }
            UI::TextWrapped(Tr("Head capture uses these slots. Saved photos keep their own scope."));
        }
        UI::Separator();
        UI::BeginDisabled(saveBusy || verificationTicks.load()!=0);
        bool compatibility=cleanupCompatibility.load();
        if(UI::Checkbox(Label("Equipment cleanup compatibility (experimental)"),&compatibility)) {
            cleanupCompatibility=compatibility; ResetTransferObservations(); SaveSettingsIfDue(true);
            SKSE::log::info("Cleanup compatibility changed: {} (applies to future outfit changes)",compatibility);
        }
        UI::EndDisabled();
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Try this if equipment created by OutfitGallery remains after changing outfits.\nIf another mod changes equipment names, first try Remove renamed generated equipment.\nAdds tracking to help remove it.\nItems returned from a chest may also be removed if confirmed to be the same generated item.\nDoes not remove items while stored elsewhere or old leftovers that are no longer tracked.\nLeave OFF if you have no problems."));
        UI::BeginDisabled(saveBusy || verificationTicks.load()!=0);
        bool renamed=reclaimRenamedGeneratedItems.load();
        if(UI::Checkbox(Label("Remove renamed generated equipment"),&renamed)) {
            reclaimRenamedGeneratedItems=renamed; SaveSettingsIfDue(true);
            SKSE::log::info("Renamed generated item cleanup changed: {}",renamed);
        }
        UI::EndDisabled();
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Removes equipment created and tracked by OutfitGallery even if its name changes.\nItems you already owned, favorites, and items you enchant or improve remain protected.\nTested with generated equipment renamed by MARA; not a guarantee of full MARA compatibility.\nTry enabling only this option first.\nIf generated equipment still remains, also try Equipment cleanup compatibility."));
        bool diagnostics=DetailEnabled();
        if(UI::Checkbox(Label("Record detailed diagnostic log"),&diagnostics)) {
            detailedLogging=diagnostics;
            ResetTransferObservations();
            SKSE::log::info("OG-DIAG session {} build=1.0.15 compatibility={}",diagnostics?"enabled":"disabled",cleanupCompatibility.load());
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Records outfit cleanup and Trash deletion details in OutfitGallery.log. Turns OFF when the game restarts. Logging does not change deletion behavior."));
        if(diagnostics) {
            UI::TextWrapped("%s",Tr("Detailed logging ON: reproduce the issue once, then send OutfitGallery.log before restarting the game."));
            if(UI::Button(Label("Show log location"))) {
                if(const auto path=SKSE::log::log_directory()) SetStatus("OutfitGallery.log: "+(path.value()/"OutfitGallery.log").string());
            }
        }

    }
    ReceiveMissingCheck();
    if(missingCheckMode) UpdateMissingCounts();
    if(compactGallery) search[0]=0;
    if(!photoRetakeActive && !compactGallery) {
    const auto* searchStyle=UI::GetStyle();
    const float checkWidth=UI::GetFrameHeight()+searchStyle->ItemInnerSpacing.x+UI::CalcTextSize(Tr("Organize gallery")).x;
    const float searchGap=std::max(searchStyle->ItemSpacing.x*2.f,UI::GetFontSize()*.65f);
    const float checkX=UI::GetCursorPosX()+UI::GetContentRegionAvail().x-checkWidth;
    UI::SetNextItemWidth(std::max(32.f,checkX-searchGap-UI::CalcTextSize(Tr("Search outfits")).x-searchStyle->ItemInnerSpacing.x-UI::GetCursorPosX()));
    UI::InputText(Label("Search outfits"),search,sizeof(search));
    UI::SameLine(); UI::SetCursorPosX(checkX);
    UI::BeginDisabled(saveBusy);
    if(UI::Button(Label("Organize gallery"))) {
        for(const auto& path:loaded) DisposePortraitView(path);
        loaded.clear(); StartOrganizer(category);
    }
    if(missingCheckFocused) UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
    UI::EndDisabled();
    }
    if(std::exchange(compactOrganizeRequested,false)) {
        for(const auto& path:loaded) DisposePortraitView(path);
        loaded.clear(); StartOrganizer(category);
    }
    if(organizer.active) {RenderOrganizer(); return;}
    if(missingCheckMode) {
        if(missingCheckPending) UI::TextColored({1.f,.8f,.35f,1.f},"%s",Tr("Missing-data check mode: checking..."));
        else if(!missingCheckError.empty()) {
            UI::TextColored({1.f,.5f,.4f,1.f},"%s",Tr("Missing-data check mode: check failed; no sets were flagged."));
            UI::TextWrapped("%s",missingCheckError.c_str());
        } else {
            UI::TextColored({1.f,.8f,.35f,1.f},Tr("Missing-data check mode: %u sets / %u in Trash"),missingCounts.active,missingCounts.trash);
        }
    }

    const bool modal=UI::IsPopupOpen(nullptr,UI::ImGuiPopupFlags_AnyPopupId);
    const bool padIntent=framePadEdges!=0;
    if(PadInputPolicy::MayReleaseFocus(framePadEdges,modal,bindingMode!=0,UI::IsMouseDown(0))) {
        UI::ClearActiveID();
    }
    const bool navigating=!photoRetakeActive && !modal && !helpOpen && !bindingMode && (!UI::GetIO()->WantTextInput || padIntent) && !UI::IsAnyItemActive() && !saveBusy;
    const bool down=PadPressed(UI::ImGuiKey_GamepadDpadDown,true)||PadPressed(UI::ImGuiKey_GamepadLStickDown,true);
    const bool up=PadPressed(UI::ImGuiKey_GamepadDpadUp,true)||PadPressed(UI::ImGuiKey_GamepadLStickUp,true);
    const bool left=PadPressed(UI::ImGuiKey_GamepadDpadLeft,true)||PadPressed(UI::ImGuiKey_GamepadLStickLeft,true);
    const bool right=PadPressed(UI::ImGuiKey_GamepadDpadRight,true)||PadPressed(UI::ImGuiKey_GamepadLStickRight,true);
    bool tabChanged=false, shortcutToPhotos=false;
    const bool browse=navigating && !reveal;
    captureAllowed=browse;
    std::vector<Category> tabs{{"","All"}};
    tabs.insert(tabs.end(),library.categories.begin(),library.categories.end());
    tabs.push_back({"#head","Head equipment"});
    tabs.push_back({"#accessories","Accessories"});
    {std::scoped_lock lock(presetsMutex); if(std::any_of(presets.begin(),presets.end(),[](const auto& p){return !p.accessories && p.slotMask && !SlotFits(p.slotMask,0x1803);})) tabs.push_back({"#legacy","Legacy parts"});}
    tabs.push_back({"#trash","Trash"});
    if(browse) {
        auto it=std::find_if(tabs.begin(),tabs.end(),[&](const auto& c){return c.id==category;});
        int idx=it==tabs.end()?0:static_cast<int>(it-tabs.begin());
        if(tabsFocused && (left || right)) {
            int target=missingCheckFocused?static_cast<int>(tabs.size())+2:layoutButtonFocused?static_cast<int>(tabs.size())+1:columnsFocused?static_cast<int>(tabs.size()):idx;
            target=(target+(right?1:-1)+static_cast<int>(tabs.size())+3)%(static_cast<int>(tabs.size())+3);
            columnsFocused=target==static_cast<int>(tabs.size());
            layoutButtonFocused=target==static_cast<int>(tabs.size())+1;
            missingCheckFocused=target==static_cast<int>(tabs.size())+2;
            if(!columnsFocused && !layoutButtonFocused && !missingCheckFocused) {idx=target; tabChanged=true;}
        }
        if(PadPressed(UI::ImGuiKey_GamepadL1,false)) {idx=(idx+static_cast<int>(tabs.size())-1)%static_cast<int>(tabs.size()); tabsFocused=true; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false; tabChanged=true;}
        if(PadPressed(UI::ImGuiKey_GamepadR1,false)) {idx=(idx+1)%static_cast<int>(tabs.size()); tabsFocused=true; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false; tabChanged=true;}
        category=tabs[idx].id;
        if(PadPressed(UI::ImGuiKey_GamepadL2,false)) {category="#head";tabsFocused=false;columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false;tabChanged=true;shortcutToPhotos=true;}
        if(PadPressed(UI::ImGuiKey_GamepadR2,false)) {category="#accessories";tabsFocused=false;columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false;tabChanged=true;shortcutToPhotos=true;}
    }
    if(browse && tabsFocused && layoutButtonFocused && PadPressed(UI::ImGuiKey_GamepadFaceDown,false)) {
        portraitThumbnails=!portraitThumbnails; thumbnailModeChanged=true; SaveSettingsIfDue(true);
    }
    if(browse && tabsFocused && missingCheckFocused && PadPressed(UI::ImGuiKey_GamepadFaceDown,false)) {
        for(const auto& path:loaded) DisposePortraitView(path);
        loaded.clear(); StartOrganizer(category); RenderOrganizer(); return;
    }
    const auto* style=UI::GetStyle();
    const float tabHeight=UI::GetTextLineHeightWithSpacing();
    if(!photoRetakeActive) {
    const auto categoryWidth=[&](const auto& c) {
        const bool icon=c.id=="#trash" || c.id=="#head" || c.id=="#accessories";
        return icon ? tabHeight+20.f : std::max(60.f,UI::CalcTextSize(CategoryTitle(c).c_str()).x+style->FramePadding.x*2);
    };
    float tabTotal=tabHeight+20.f; // Column button, plus one gap per category.
    for(const auto& c:tabs) tabTotal+=categoryWidth(c)+style->ItemSpacing.x;
    const bool tabOverflow=tabTotal>UI::GetContentRegionAvail().x-style->WindowPadding.x*2;
    const float stripHeight=tabHeight+style->WindowPadding.y*2+(tabOverflow?style->ScrollbarSize:0);
    UI::BeginChild("Categories",{0,stripHeight},0,UI::ImGuiWindowFlags_HorizontalScrollbar|UI::ImGuiWindowFlags_NoNavInputs);
    for(std::size_t n=0;n<tabs.size();++n) {
        if(n) UI::SameLine();
        const auto& c=tabs[n];
        const float tabWidth=categoryWidth(c);
        const bool icon=c.id=="#trash" || c.id=="#head" || c.id=="#accessories";
        UI::PushStyleColor(UI::ImGuiCol_Header,{.42f,.29f,.07f,1.f});
        UI::PushStyleColor(UI::ImGuiCol_HeaderHovered,{.55f,.38f,.10f,1.f});
        UI::PushStyleColor(UI::ImGuiCol_HeaderActive,{.65f,.46f,.12f,1.f});
        if(UI::Selectable((icon?"##icon"+c.id:CategoryTitle(c)+"##tab"+c.id).c_str(),category==c.id,0,{tabWidth,tabHeight})) {category=c.id; tabsFocused=true; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false; tabChanged=true;}
        UI::PopStyleColor(3);
        if(category==c.id) {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax(); auto* draw=UI::GetWindowDrawList();
            UI::ImDrawListManager::AddLine(draw,{a.x+2,b.y-2},{b.x-2,b.y-2},0xFF30C8FF,3.f);

        }
        if(UI::IsItemHovered()) {
            const auto shortcut=c.id=="#head" ? "\nLT / L2" : c.id=="#accessories" ? "\nRT / R2" : "";
            const char* explanation=c.id=="#head" ? "Person icon: change only head equipment, keeping your clothes." :
                c.id=="#accessories" ? "Ring icon: add the pictured items, keeping your clothes; replace items in matching slots." :
                (c.id=="#trash" || c.id=="#legacy") ? nullptr : "Text tabs: replace the full outfit (organize with tabs).";
            if(explanation) UI::SetTooltip("%s\n%s%s",CategoryTitle(c).c_str(),Tr(explanation),shortcut);
            else UI::SetTooltip("%s%s",CategoryTitle(c).c_str(),shortcut);
        }
        if(c.id=="#accessories") {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
            auto* draw=UI::GetWindowDrawList(); const float x=(a.x+b.x)*.5f, r=tabHeight*.27f;
            UI::ImDrawListManager::AddCircle(draw,{x,a.y+tabHeight*.60f},r,0xFFE0E0E0,20,2.f);
            const float y=a.y+tabHeight*.18f,d=tabHeight*.12f;
            UI::ImDrawListManager::AddLine(draw,{x-d,y},{x,y-d},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x,y-d},{x+d,y},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+d,y},{x,y+d},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x,y+d},{x-d,y},0xFFE0E0E0,2.f);
        }
        if(c.id=="#head") {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
            auto* draw=UI::GetWindowDrawList(); const float x=(a.x+b.x)*.5f, r=tabHeight*.19f;
            UI::ImDrawListManager::AddCircle(draw,{x,a.y+tabHeight*.30f},r,0xFFE0E0E0,16,2.f);
            UI::ImDrawListManager::AddLine(draw,{x-r*1.8f,a.y+tabHeight*.85f},{x-r,a.y+tabHeight*.59f},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x-r,a.y+tabHeight*.59f},{x+r,a.y+tabHeight*.59f},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+r,a.y+tabHeight*.59f},{x+r*1.8f,a.y+tabHeight*.85f},0xFFE0E0E0,2.f);
        }
        if(c.id=="#trash") {
            const auto a=UI::GetItemRectMin(); auto* draw=UI::GetWindowDrawList();
            const float unit=tabHeight/20.f, x=a.x+7.f, y=a.y+2.f;
            UI::ImDrawListManager::AddRect(draw,{x+3*unit,y+5*unit},{x+15*unit,y+17*unit},0xFFE0E0E0,1.f,0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+1*unit,y+3*unit},{x+17*unit,y+3*unit},0xFFE0E0E0,2.f);
            UI::ImDrawListManager::AddLine(draw,{x+6*unit,y},{x+12*unit,y},0xFFE0E0E0,2.f);
            for(int k=7;k<=11;k+=4) UI::ImDrawListManager::AddLine(draw,{x+k*unit,y+8*unit},{x+k*unit,y+14*unit},0xFFE0E0E0,1.5f);
            if(UI::BeginPopupContextItem("Trash actions")) {
                if(UI::MenuItem(Label("Empty Trash"),nullptr,false,libraryWritable&&!saveBusy)) {
                    deletePhotos.clear(); for(const auto& [photo,label]:library.labels) if(label.deleted) deletePhotos.push_back(photo);
                    openDelete=!deletePhotos.empty();
                }
                UI::EndPopup();
            }
        }
        UI::PushStyleColor(UI::ImGuiCol_DragDropTarget,{1.f,.91f,.44f,1.f});
        if(!c.id.empty() && (!c.id.starts_with("#") || c.id=="#trash") && libraryWritable && !saveBusy && UI::BeginDragDropTarget()) {
            if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_PHOTO")) {
                if(payload->Data && payload->DataSize>1 && payload->DataSize<512 && static_cast<const char*>(payload->Data)[payload->DataSize-1]=='\0') {
                    const std::string photo(static_cast<const char*>(payload->Data));
                    auto next=library; auto& label=next.labels[photo];
                    if(c.id=="#trash") {label.deleted=true; CommitLibrary(std::move(next));}
                    else if(!label.deleted && [&]{std::scoped_lock lock(presetsMutex); return std::any_of(presets.begin(),presets.end(),[&](const auto& p){return p.photo==photo && !p.slotMask && !p.accessories;});}()) {if(std::find(label.categories.begin(),label.categories.end(),c.id)==label.categories.end()) label.categories.push_back(c.id); CommitLibrary(std::move(next));}
                }
            }
            UI::EndDragDropTarget();
        }
        UI::PopStyleColor();
        if(!c.id.empty() && !c.id.starts_with("#") && UI::IsItemClicked(1)) {
            editCategory=c.id; newCategory=false; strcpy_s(tabName,c.name.c_str()); openTab=true;
        }
        if(category==c.id && (tabChanged || reveal)) UI::SetScrollHereX(.5f);
    }
    UI::SameLine();
    const bool cycleWithPad=browse && tabsFocused && columnsFocused && PadPressed(UI::ImGuiKey_GamepadFaceDown,false);
    const bool cycleWithStart=browse && gamepadHotkey!=16 && autoGamepad!=16 &&
        capturePads[0]!=16 && capturePads[1]!=16 && capturePads[2]!=16 && RawPadPressed(16,false);
    if(UI::Button("##cycleColumns",{tabHeight+20.f,tabHeight}) || cycleWithPad || cycleWithStart) {gridColumns=gridColumns==6?2:gridColumns+1; SaveSettingsIfDue(true);}
    if(tabsFocused && columnsFocused) {
        UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
        if(left || right) UI::SetScrollHereX(.5f);
    }
    DrawColumnCountIcon(tabHeight);
    if(UI::IsItemHovered()) UI::SetTooltip(Tr("Columns: %d (click to cycle)"),ThumbnailColumns(gridColumns,portraitThumbnails));
    UI::EndChild();
    }
    std::vector<Preset> cards;
    {std::scoped_lock lock(presetsMutex); cards=presets;}
    std::map<std::string,int> ranks;
    for(std::size_t n=0;n<library.order.size();++n) ranks[library.order[n]]=static_cast<int>(n);
    auto rank=[&](const auto& p){const auto it=ranks.find(p.photo); return it==ranks.end()?-1:it->second;};
    std::stable_sort(cards.begin(),cards.end(),[&](const auto& a,const auto& b){return rank(a)<rank(b);});
    std::vector<std::string> fullOrder;
    for(const auto& p:cards) fullOrder.push_back(p.photo);
    for(auto& p:cards) {
        const auto it=library.labels.find(p.photo);
        if(it!=library.labels.end() && !it->second.name.empty()) p.name=it->second.name;
    }
    std::erase_if(cards,[&](const auto& p){
        const auto it=library.labels.find(p.photo);
        const bool deleted=it!=library.labels.end() && it->second.deleted;
        if(deleted!=(category=="#trash")) return true;
        if(!CollectionFits(p.slotMask,category,p.accessories)) return true;
        if(!category.empty() && !category.starts_with("#") && (it==library.labels.end() || std::find(it->second.categories.begin(),it->second.categories.end(),category)==it->second.categories.end())) return true;
        return search[0] && p.name.find(search)==std::string::npos;
    });
    if(missingCheckMode && !missingCheckPending && missingCheckError.empty()) MissingPresetsFirst(cards,missingResults);
    if(cards.empty()) {tabsFocused=true; selected.clear();}
    const int displayColumns=ThumbnailColumns(gridColumns,portraitThumbnails);
    const bool modeChanged=thumbnailModeChanged; thumbnailModeChanged=false;
    const auto filter=category+"|"+search+"|"+std::to_string(gridColumns)+std::to_string(showNames)+std::to_string(showCounts)+"|missing="+std::to_string(missingOrderSerial);
    const bool reset=filter!=oldFilter; oldFilter=filter;
    int index=0;
    auto chosen=std::find_if(cards.begin(),cards.end(),[&](const auto& p){return p.photo==selected;});
    if(chosen!=cards.end() && !tabChanged) index=static_cast<int>(chosen-cards.begin());
    const int before=index;
    const bool revealRetake=std::exchange(photoRetakeReveal,false) || photoRetakeLayoutFrames>0;
    if(photoRetakeLayoutFrames) --photoRetakeLayoutFrames;
    if(photoRetakeActive) {
        const auto found=std::find_if(cards.begin(),cards.end(),[&](const auto& p){return p.photo==photoUpdate.Photo();});
        if(found!=cards.end()) {index=static_cast<int>(found-cards.begin()); tabsFocused=false;}
    }
    bool moved=revealRetake || reveal.has_value() || modeChanged || shortcutToPhotos;
    bool enteredPhotos=shortcutToPhotos;
    if(browse && !cards.empty() && tabsFocused && (down || (!columnsFocused && !layoutButtonFocused && !missingCheckFocused && PadPressed(UI::ImGuiKey_GamepadFaceDown,false)))) {tabsFocused=false; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false; index=0; moved=true; enteredPhotos=true;}
    if(browse && !cards.empty() && !tabsFocused && !enteredPhotos && !tabChanged) {
        if(down) {index+=displayColumns; moved=true;}
        if(up) {if(index<displayColumns) tabsFocused=true; else index-=displayColumns; moved=true;}
        if(left) {--index; moved=true;}
        if(right) {++index; moved=true;}


        index=std::clamp(index,0,static_cast<int>(cards.size())-1);
    }
    if(!cards.empty()) {selected=cards[index].photo; if(autoPanel) autoSelectedPhoto=selected;}
    auto queueApply=[&](const Preset& p){
        if(photoRetakeActive || saveBusy || category=="#trash") return;
        if(autoPanel) {autoSelectedPhoto=p.photo; return;}
        std::scoped_lock lock(presetsMutex); requestedPreset=p;
        {const auto label=library.labels.find(p.photo); if(label!=library.labels.end()) {requestedPreset.exchangeSlots=label->second.exchangeSlots;requestedPreset.exchangeSlotless=label->second.exchangeSlotless;requestedPreset.outfitItems=label->second.outfitItems;}}
        requestedAddMissing=addMissing; command=4;
    };
    if(browse && !cards.empty() && !tabsFocused && !enteredPhotos) {
        if(PadPressed(UI::ImGuiKey_GamepadFaceDown,false)) queueApply(cards[index]);
        if(PadPressed(UI::ImGuiKey_GamepadR3,false)) {contextPhoto=cards[index].photo; strcpy_s(editName,cards[index].name.c_str()); openEdit=true;}
    }
    if(browse && tabsFocused && !columnsFocused && !layoutButtonFocused && !missingCheckFocused && PadPressed(UI::ImGuiKey_GamepadR3,false) && !category.empty() && !category.starts_with("#")) {
        auto it=std::find_if(library.categories.begin(),library.categories.end(),[&](const auto& c){return c.id==category;});
        if(it!=library.categories.end()) {newCategory=false; editCategory=category; strcpy_s(tabName,it->name.c_str()); openTab=true;}
    }
    if(browse && PadPressed(UI::ImGuiKey_GamepadFaceRight,false)) { GALLERY_DIAG("OG-OPEN-DIAG gallery B-close"); command=2; }
    if(browse && RawPadPressed(capturePads[1]) && !saveBusy.exchange(true)) {
        {std::scoped_lock lock(presetsMutex); requestedName.clear();} command=5;
    }
    if(browse && RawPadPressed(capturePads[2]) && !saveBusy.exchange(true)) {
        {std::scoped_lock lock(presetsMutex); requestedName.clear();} command=6;
    }
    if(browse && RawPadPressed(capturePads[0])) {
        if(!saveBusy.exchange(true)) {
            {std::scoped_lock lock(presetsMutex); requestedName.clear();}
            command=3;
        }
    }
    const auto screen=UI::GetIO()->DisplaySize;
    const float footer=UI::GetTextLineHeightWithSpacing()*1.4f;
    // Reserve scrollbar width even when all rows fit. Otherwise its appearance
    // narrows the thumbnails, making them shorter and hiding it again next frame.
    UI::BeginChild("Presets",{0,std::max(80.f,UI::GetContentRegionAvail().y-footer)},0,
        UI::ImGuiWindowFlags_NoNavInputs|UI::ImGuiWindowFlags_AlwaysVerticalScrollbar);
    const auto layout=MakeThumbnailLayout(UI::GetContentRegionAvail().x,screen.x,screen.y,gridColumns,portraitThumbnails);
    const float stepX=layout.stepX, w=layout.width, h=layout.height;
    const UI::ImVec2 uv0{layout.uvLeft,0.f}, uv1{layout.uvRight,1.f};
    thumbnailVisibleWidth=layout.uvRight-layout.uvLeft;
    const float textHeight=UI::GetTextLineHeightWithSpacing();
    const float stepY=h+16.f+(showNames?textHeight:0)+(showCounts?textHeight:0);
    const int totalRows=(static_cast<int>(cards.size())+displayColumns-1)/displayColumns;
    float scroll=reset?0.f:UI::GetScrollY();
    const float viewport=std::max(1.f,UI::GetWindowHeight()-16.f);
    if(reset) UI::SetScrollY(0);
    if(moved || index!=before) {
        const float y=(index/displayColumns)*stepY;
        if(y<scroll) scroll=y; else if(y+stepY>scroll+viewport) scroll=y+stepY-viewport;
        scroll=std::max(0.f,scroll); UI::SetScrollY(scroll);
    }
    const int start=std::clamp(static_cast<int>(scroll/stepY)-1,0,totalRows);
    const int finish=std::min(totalRows,start+static_cast<int>(viewport/stepY)+4);
    std::vector<std::string> visible;
    const float origin=UI::GetCursorPosY();
    UI::BeginDisabled(saveBusy);
    for(int row=start;row<finish;++row) for(int col=0;col<displayColumns;++col) {
        const int n=row*displayColumns+col; if(n>=static_cast<int>(cards.size())) break;
        const auto& p=cards[n];
        UI::SetCursorPos({8.f+col*stepX,origin+row*stepY});
        UI::BeginGroup();
        const auto path=(photoRetakeActive && p.photo==photoUpdate.Photo() && !photoUpdate.Draft().empty() ? photoUpdate.Draft() : std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/p.photo).generic_string();
        visible.push_back(path);
        const auto texture=LoadPortraitView(path);
        if(p.photo==selected && !tabsFocused) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
        // The overlaid trash X must be able to receive hover/clicks instead of
        // the full-size image button claiming that area first.
        const bool flagged=missingCheckMode && PresetHasMissingData(missingResults,p.photo);
        if(category=="#trash" || flagged) UI::SetNextItemAllowOverlap();
        // The grid reserves 12 px beyond the image width. Keep the button's
        // padding local and bounded so a theme's FramePadding cannot make the
        // next image cover the selection border. Match missing-photo bounds.
        const UI::ImVec2 photoPadding{3.f,3.f};
        UI::PushStyleVar(UI::ImGuiStyleVar_FramePadding,photoPadding);
        const bool clicked=texture?UI::ImageButton(p.photo.c_str(),texture,{w,h},uv0,uv1):UI::Button((std::string(Tr("Missing photo"))+"##"+p.photo).c_str(),{w+6.f,h+6.f});
        UI::PopStyleVar();
        if(p.photo==selected && !tabsFocused) UI::PopStyleColor();
        const bool hover=UI::IsItemHovered();
        const auto photoMin=UI::GetItemRectMin(), photoMax=UI::GetItemRectMax();
        if(p.photo==selected && !tabsFocused) {
            auto* draw=UI::GetWindowDrawList();
            UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF171717,0,0,7.f);
            UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF70C6E8,0,0,4.f);
        }
        const auto label=library.labels.find(p.photo);
        // A full outfit override changes full replacement into selected-slot
        // replacement even when all saved slots are selected. For partial
        // outfits an explicit selection equal to the original is unchanged.
        const bool editedScope=label!=library.labels.end() && (label->second.outfitItems.has_value() || (label->second.exchangeSlots.has_value() &&
            (!p.slotMask || *label->second.exchangeSlots!=p.slotMask)));
        if(editedScope) {
            auto* draw=UI::GetWindowDrawList();
            const float radius=std::clamp(w*.024f,3.5f,5.5f);
            // ImageButton's item rectangle includes frame padding. Anchor to
            // the actual image so the halo stays fully inside the photograph.
            const auto padding=photoPadding;
            const UI::ImVec2 center{photoMin.x+padding.x+radius+3.f,photoMin.y+padding.y+radius+3.f};
            UI::ImDrawListManager::AddCircleFilled(draw,center,radius+1.5f,0xD0202020,16);
            UI::ImDrawListManager::AddCircleFilled(draw,center,radius,0xFFEBDD9A,16);
        }
        const auto pad=photoPadding;
        const float missingRadius=std::clamp(w*.055f,8.f,15.f);
        const float editedRadius=std::clamp(w*.024f,3.5f,5.5f);
        const UI::ImVec2 missingCenter{photoMin.x+pad.x+missingRadius+3.f+(editedScope?editedRadius*2.f+8.f:0.f),photoMin.y+pad.y+missingRadius+3.f};
        const float missingHalf=missingRadius+2.f;
        const auto mouse=UI::GetIO()->MousePos;
        const bool overMissing=flagged && mouse.x>=missingCenter.x-missingHalf && mouse.x<=missingCenter.x+missingHalf && mouse.y>=missingCenter.y-missingHalf && mouse.y<=missingCenter.y+missingHalf;
        if(!p.slotMask && !p.accessories && label!=library.labels.end() && std::find(label->second.categories.begin(),label->second.categories.end(),"favorites")!=label->second.categories.end()) {
            auto* draw=UI::GetWindowDrawList();
            const float radius=std::clamp(w*.055f,8.f,15.f);
            const UI::ImVec2 center{photoMax.x-radius-6.f,photoMin.y+radius+6.f};
            UI::ImDrawListManager::AddCircleFilled(draw,center,radius+3.f,0xD0202020,20);
            UI::ImVec2 points[10];
            for(int k=0;k<10;++k) {const float angle=-std::numbers::pi_v<float>/2+k*std::numbers::pi_v<float>/5; const float r=k%2?radius*.43f:radius; points[k]={center.x+std::cos(angle)*r,center.y+std::sin(angle)*r};}
            for(int k=0;k<10;++k) UI::ImDrawListManager::AddLine(draw,points[k],points[(k+1)%10],0xFF55DFFF,2.f);
        }
        if(hover && !overMissing) UI::SetTooltip("%s",p.name.c_str());
        if(!photoRetakeActive && UI::IsItemClicked(1)) {contextPhoto=p.photo; strcpy_s(editName,p.name.c_str()); openEdit=true; selected=p.photo;}
        // Gamepad A is handled once above; avoid native-nav activation applying another card.
        if(clicked && !overMissing && !UI::GetDragDropPayload() && !PadPressed(UI::ImGuiKey_GamepadFaceDown,false)) {selected=p.photo; tabsFocused=false; columnsFocused=false; layoutButtonFocused=false; missingCheckFocused=false; queueApply(p);}
        UI::PushStyleColor(UI::ImGuiCol_PopupBg,{.07f,.07f,.07f,1.f});
        // Only photo dragging gets extra tolerance; do not modify ImGui's global
        // threshold. A slight movement still releases as a normal ImageButton click.
        // IsMouseDragging uses the maximum distance since mouse-down, retaining
        // the drag once started even when the cursor moves back toward its origin.
        const float photoDragThreshold=std::max(36.f,UI::GetIO()->MouseDragThreshold);
        if(!photoRetakeActive && !missingCheckMode && libraryWritable && UI::IsMouseDragging(0,photoDragThreshold) && UI::BeginDragDropSource()) {
            UI::SetDragDropPayload("OUTFIT_PHOTO",p.photo.c_str(),p.photo.size()+1);
            UI::TextUnformatted(p.name.c_str());
            if(texture) UI::Image(texture,{w*.5f,h*.5f},uv0,uv1);
            UI::EndDragDropSource();
        }
        UI::PopStyleColor();
        if(!missingCheckMode && libraryWritable && UI::BeginDragDropTarget()) {
            if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_PHOTO",UI::ImGuiDragDropFlags_AcceptBeforeDelivery|UI::ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                if(payload->Data && payload->DataSize>1 && payload->DataSize<512 && static_cast<const char*>(payload->Data)[payload->DataSize-1]=='\0') {
                    const std::string source(static_cast<const char*>(payload->Data));
                    const bool after=UI::GetIO()->MousePos.x>(photoMin.x+photoMax.x)*.5f;
                    if(source!=p.photo && payload->Preview) {
                        auto* draw=UI::GetWindowDrawList();
                        UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF171717,0,0,7.f);
                        UI::ImDrawListManager::AddRect(draw,{photoMin.x+3,photoMin.y+3},{photoMax.x-3,photoMax.y-3},0xFF70E8FF,0,0,3.f);
                        const float x=after?photoMax.x-5:photoMin.x+5;
                        UI::ImDrawListManager::AddLine(draw,{x,photoMin.y+5},{x,photoMax.y-5},0xFF171717,10.f);
                        UI::ImDrawListManager::AddLine(draw,{x,photoMin.y+5},{x,photoMax.y-5},0xFF70E8FF,6.f);
                    }
                    if(payload->Delivery) {
                        auto next=library; next.order=fullOrder;
                        if(MovePhoto(next.order,source,p.photo,after) && CommitLibrary(std::move(next))) selected=source;
                    }
                }
            }
            UI::EndDragDropTarget();
        }
        if(flagged) {
            const auto cursor=UI::GetCursorPos();
            UI::SetCursorScreenPos({missingCenter.x-missingHalf,missingCenter.y-missingHalf});
            UI::BeginDisabled(!libraryWritable || category=="#trash");
            if(UI::InvisibleButton(("##missingTrash"+p.photo).c_str(),{missingHalf*2.f,missingHalf*2.f}) && !UI::GetDragDropPayload()) moveMissingToTrash=p.photo;
            const bool missingHovered=UI::IsItemHovered();
            UI::EndDisabled();
            auto* draw=UI::GetWindowDrawList();
            UI::ImDrawListManager::AddCircleFilled(draw,missingCenter,missingHalf,missingHovered?0xF0404050:0xE0202020,20);
            const float d=missingRadius*.65f;
            UI::ImDrawListManager::AddLine(draw,{missingCenter.x-d,missingCenter.y-d},{missingCenter.x+d,missingCenter.y+d},0xFF8080FF,2.f);
            UI::ImDrawListManager::AddLine(draw,{missingCenter.x-d,missingCenter.y+d},{missingCenter.x+d,missingCenter.y-d},0xFF8080FF,2.f);
            if(missingHovered) UI::SetTooltip("%s",Tr("Move to Trash"));
            UI::SetCursorPos(cursor);
        }
        if(category=="#trash") {
            const auto cursor=UI::GetCursorPos();
            const float size=UI::GetFrameHeight();
            UI::SetCursorScreenPos({photoMax.x-size-3.f,photoMin.y+3.f});
            UI::BeginDisabled(!libraryWritable);
            if(UI::Button(("X##delete"+p.photo).c_str(),{size,size})) {deletePhotos={p.photo}; openDelete=true;}
            UI::EndDisabled();
            UI::SetCursorPos(cursor);
        }
        if(showNames) {
            auto shortName=p.name;
            if(UI::CalcTextSize(shortName.c_str()).x>w) {
                while(!shortName.empty() && UI::CalcTextSize((shortName+"...").c_str()).x>w) {
                    auto last=shortName.size()-1;
                    while(last && (static_cast<unsigned char>(shortName[last])&0xC0)==0x80) --last;
                    shortName.resize(last);
                }
                shortName+="...";
            }
            UI::TextUnformatted(shortName.c_str());
        }
        if(showCounts) UI::Text(Tr("%u items"),static_cast<unsigned>(p.items.size()));
        UI::EndGroup();
    }
    UI::EndDisabled();
    UI::SetCursorPos({8,origin+totalRows*stepY}); UI::Dummy({1,1});
    UI::EndChild();
    if(!moveMissingToTrash.empty()) {
        auto next=library; next.labels[moveMissingToTrash].deleted=true;
        CommitLibrary(std::move(next));
    }
    if(autoPanel) {
        std::scoped_lock lock(autoMutex);
        for(const auto& photo:autoSlots) if(!photo.empty()) visible.push_back((std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/photo).generic_string());
    }
    for(const auto& path:loaded) if(std::find(visible.begin(),visible.end(),path)==visible.end()) DisposePortraitView(path);
    loaded=std::move(visible);
    auto current=std::find_if(cards.begin(),cards.end(),[&](const auto& p){return p.photo==selected;});
    if(current!=cards.end()) UI::Text(Tr("%s  (%u items)"),current->name.c_str(),static_cast<unsigned>(current->items.size()));
    else UI::TextUnformatted(Tr("No outfits in this view."));
    if(current!=cards.end() && (current->slotMask || current->accessories)) {UI::SameLine(); UI::TextUnformatted(Tr(current->accessories?"[Accessories]":"[Selected slots]"));}
    UI::SameLine(0,24); UI::Text(Tr("%u outfits"),static_cast<unsigned>(cards.size()));
    if(openDelete) UI::OpenPopup(Label("Confirm permanent deletion"));
    if(UI::BeginPopupModal(Label("Confirm permanent deletion"),nullptr,UI::ImGuiWindowFlags_AlwaysAutoResize)) {
        UI::TextWrapped(Tr("Permanently delete these presets, photos and their edit backups? This cannot be undone."));
        UI::Text(Tr("%u outfits"),static_cast<unsigned>(deletePhotos.size()));
        if(UI::Button(Label("Delete permanently"))) {
            try {
                for(const auto& photo:deletePhotos) {
                    const auto path=(std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/photo).generic_string();
                    DisposePortraitView(path);
                    DeletionTrace trace;
                    if(DetailEnabled()) trace=[](const std::string& line){GALLERY_DIAG("OG-DIAG trash {}",line);};
                    DeleteTrashedFiles(photo,library,"Data/SKSE/Plugins/OutfitGallery",std::move(trace));
                    auto next=library; next.labels.erase(photo); std::erase(next.order,photo);
                    if(!CommitLibrary(std::move(next))) break;
                }
            }catch(const std::exception& e){SetStatus(e.what());}
            try {ReloadPresets();}catch(const std::exception& e){SetStatus(e.what());}
            deletePhotos.clear(); UI::CloseCurrentPopup();
        }
        UI::SameLine();
        if(UI::Button(Label("Cancel"))) {deletePhotos.clear(); UI::CloseCurrentPopup();}
        UI::EndPopup();
    }
    if(openEdit) UI::OpenPopup("Outfit actions");
    if(UI::BeginPopup("Outfit actions")) {
        if(missingCheckMode && !missingCheckPending && missingCheckError.empty()) {
            const auto result=missingResults.find(contextPhoto);
            if(result==missingResults.end()) UI::TextWrapped("%s",Tr("Not checked in this snapshot."));
            else if(!result->second.empty()) {
                UI::TextColored({1.f,.5f,.4f,1.f},"%s",Tr("Missing or changed equipment data"));
                UI::BeginChild("Missing details",{540.f,std::min(200.f,UI::GetTextLineHeightWithSpacing()*(1.f+result->second.size()*2.f))},0);
                for(const auto& item:result->second) {
                    UI::TextWrapped("%s",item.name.c_str());
                    UI::TextWrapped("%s / %06X",item.plugin.c_str(),item.localID);
                }
                UI::EndChild(); UI::Separator();
            }
        }
        if(UI::MenuItem(Label("Retake photo"),nullptr,false,category!="#trash" && !saveBusy && !verificationTicks.load() && !editPending && !command.load())) {
            try {
                photoUpdate.Begin(contextPhoto);
                photoRetakeName=editName;
                SetStatus("");
                photoRetakeSerial=studioOpenSerial.load(); photoRetakeActive=true;
                captureAllowed=false; autoPanel=false; autoPanelVisible=false; helpOpen=false;
                photoRetakeReveal=true;
                UI::CloseCurrentPopup();
            } catch(const std::exception& e) {SetStatus(e.what());}
        }
        UI::Separator();
        UI::InputText(Label("Name"),editName,sizeof(editName));
        if(UI::Button(Label("Rename")) && editName[0]) {auto next=library; next.labels[contextPhoto].name=editName; CommitLibrary(std::move(next));}
        UI::Separator();
        if(UI::Button(Label("New tab"))) {newCategory=true; tabName[0]=0; openTab=true; UI::CloseCurrentPopup();}
        if(!category.empty() && !category.starts_with("#") && UI::Button(Label("Edit current tab"))) {
            auto it=std::find_if(library.categories.begin(),library.categories.end(),[&](const auto& c){return c.id==category;});
            if(it!=library.categories.end()) {newCategory=false; editCategory=category; strcpy_s(tabName,it->name.c_str()); openTab=true; UI::CloseCurrentPopup();}
        }
        bool partialContext=false;
        {std::scoped_lock lock(presetsMutex); for(const auto& p:presets) if(p.photo==contextPhoto) partialContext=p.slotMask!=0 || p.accessories;}
        if(UI::Button(Label("Equipment management"))) {
            {std::scoped_lock lock(editMutex);editSnapshot={};}
            {std::scoped_lock lock(presetsMutex);
                for(const auto& p:presets) if(p.photo==contextPhoto) {scopeEditing=p; break;}
                scopeRequest=scopeEditing; scopeReady=false; scopeMasks.clear(); scopeError.clear();
            }
            const auto label=library.labels.find(contextPhoto);
            scopeItems=label!=library.labels.end()?label->second.outfitItems:std::nullopt;
            scopeDraftInitialized=label!=library.labels.end() && label->second.exchangeSlots.has_value();
            scopeDraft=scopeDraftInitialized ? *label->second.exchangeSlots : 0;
            scopeSlotless=scopeDraftInitialized?label->second.exchangeSlotless:std::vector<std::string>{};
            command=10; openScope=true; UI::CloseCurrentPopup();
        }
        bool contentEdited=false;
        {std::scoped_lock lock(presetsMutex);for(const auto& p:presets)if(p.photo==contextPhoto)contentEdited=p.equipmentEdited;}
        if(contentEdited) {UI::SameLine();UI::TextUnformatted(Tr("Equipment edited"));}
        if(!partialContext) UI::TextUnformatted(Tr("Categories (multiple allowed)"));
        if(!partialContext) for(const auto& c:library.categories) {
            const auto found=library.labels.find(contextPhoto);
            bool member=found!=library.labels.end() && std::find(found->second.categories.begin(),found->second.categories.end(),c.id)!=found->second.categories.end();
            if(UI::Checkbox((CategoryTitle(c)+"##"+c.id).c_str(),&member)) {
                auto next=library; auto& list=next.labels[contextPhoto].categories;
                if(member) list.push_back(c.id); else std::erase(list,c.id);
                CommitLibrary(std::move(next)); break;
            }
        }
        const auto found=library.labels.find(contextPhoto);
        const bool deleted=found!=library.labels.end()&&found->second.deleted;
        if(UI::Button(Label(deleted?"Restore from Trash":"Move to Trash"))) {auto next=library; next.labels[contextPhoto].deleted=!deleted; if(CommitLibrary(std::move(next))) UI::CloseCurrentPopup();}
        if(UI::Button(Label("Close"))) UI::CloseCurrentPopup();
        UI::EndPopup();
    }
    if(openScope) UI::OpenPopup(Label("Equipment management"));
    PrepareEquipmentDialog();
    bool managementOpen=true;
    if(UI::BeginPopupModal(Label("Equipment management"),editPending?nullptr:&managementOpen,UI::ImGuiWindowFlags_NoSavedSettings)) {
        UI::BeginChild("Management body",{0,-UI::GetFrameHeightWithSpacing()-UI::GetStyle()->ItemSpacing.y});
        UI::TextUnformatted(scopeEditing.name.c_str());
        if(scopeEditing.equipmentEdited) {UI::SameLine();UI::TextUnformatted(Tr("Equipment edited"));}
        // Measure the parent before entering the auto-sized section. Reserve the
        // controls below; give the remaining height to the equipment list.
        const float row=UI::GetFrameHeightWithSpacing();
        const float lowerRows=(scopeEditing.slotMask || scopeEditing.accessories)?10.f:9.f;
        const float scopeChoicesHeight=std::max(row*4.f,UI::GetContentRegionAvail().y-row*lowerRows-UI::GetStyle()->ItemSpacing.y*8.f);
        const bool fullOutfit=!scopeEditing.slotMask && !scopeEditing.accessories;
        UI::Spacing();BeginEquipmentSection("Scope section",fullOutfit?"Equipment to wear":"Exchange slots",false);
        bool readyScope; std::vector<std::uint32_t> masks; std::string error;
        {std::scoped_lock lock(presetsMutex); readyScope=scopeReady; masks=scopeMasks; error=scopeError;}
        if(!readyScope) UI::TextUnformatted(Tr("Loading equipment slots..."));
        else if(!error.empty()) UI::TextWrapped("%s",StatusText(error).c_str());
        else {
            const auto available=AvailableExchangeSlots(scopeEditing,masks);
            std::vector<std::string> allSlotless;
            for(std::size_t n=0;n<masks.size();++n) if(!masks[n] && scopeEditing.items[n].kind=="armor") allSlotless.push_back(ExchangeItemKey(scopeEditing.items[n]));
            if(!scopeDraftInitialized) {scopeDraft=available;scopeSlotless=allSlotless;scopeDraftInitialized=true;}
            if(fullOutfit && !scopeItems) {
                auto legacy=scopeEditing;
                if(scopeDraftInitialized) {legacy.exchangeSlots=scopeDraft;legacy.exchangeSlotless=scopeSlotless;}
                const auto filtered=FilterExchangeSlots(legacy,masks);
                scopeItems=std::vector<std::string>{};
                for(const auto& item:filtered.items) if(item.kind=="armor") scopeItems->push_back(ExchangeItemKey(item));
            }
            UI::BeginChild("Scope choices",{0,scopeChoicesHeight});
            if(fullOutfit) {
                for(std::size_t n=0;n<scopeEditing.items.size();++n) {
                    const auto& item=scopeEditing.items[n]; if(item.kind!="armor") continue;
                    const auto key=ExchangeItemKey(item);
                    bool checked=std::find(scopeItems->begin(),scopeItems->end(),key)!=scopeItems->end();
                    UI::PushID(static_cast<int>(n));
                    std::string slots;
                    for(unsigned bit=0;bit<32;++bit) if(masks[n]&(1u<<bit)) {
                        if(!slots.empty()) slots+=", ";
                        slots+=std::to_string(bit+30);
                    }
                    const auto itemLabel="["+(slots.empty()?std::string(Tr("No biped slot")):slots)+"] "+item.name;
                    if(UI::Checkbox(itemLabel.c_str(),&checked)) {if(checked) scopeItems->push_back(key);else std::erase(*scopeItems,key);}
                    UI::PopID();
                }
            } else {
            for(unsigned bit=0;bit<32;++bit) if(available&(1u<<bit)) {
                bool checked=(scopeDraft&(1u<<bit))!=0;
                if(UI::Checkbox(std::to_string(bit+30).c_str(),&checked)) {
                    scopeDraft=ToggleExchangeSlot(scopeDraft,1u<<bit,checked,masks);
                }
                for(std::size_t n=0;n<masks.size();++n) if(masks[n]&(1u<<bit)) {UI::SameLine(); UI::TextWrapped("%s",scopeEditing.items[n].name.c_str());}
            }
            for(std::size_t n=0;n<masks.size();++n) if(!masks[n] && scopeEditing.items[n].kind=="armor") {
                const auto key=ExchangeItemKey(scopeEditing.items[n]);
                bool checked=std::find(scopeSlotless.begin(),scopeSlotless.end(),key)!=scopeSlotless.end();
                UI::PushID(static_cast<int>(n));
                if(UI::Checkbox(scopeEditing.items[n].name.c_str(),&checked)) {if(checked) scopeSlotless.push_back(key);else std::erase(scopeSlotless,key);}
                UI::PopID();UI::SameLine();UI::TextUnformatted(Tr("Slotless equipment"));
            }
            }
            UI::EndChild();
            if(UI::Button(Label("Select all slots"))) {scopeDraft=available;scopeSlotless=allSlotless;if(fullOutfit) {scopeItems=std::vector<std::string>{};for(const auto& item:scopeEditing.items) if(item.kind=="armor") scopeItems->push_back(ExchangeItemKey(item));}}
            UI::SameLine(); if(UI::Button(Label("Clear slot selection"))) {scopeDraft=0;scopeSlotless.clear();if(fullOutfit) scopeItems=std::vector<std::string>{};}
            if(fullOutfit?scopeItems->empty():(!scopeDraft && scopeSlotless.empty())) UI::TextUnformatted(Tr(fullOutfit?"No equipment selected. Equipment unchanged.":"No exchange slots selected. Equipment unchanged."));
            if(UI::Button(Label(fullOutfit?"Save equipment selection":"Save exchange slots"))) {
                auto next=library;
                if(fullOutfit) {auto& label=next.labels[scopeEditing.photo];label.outfitItems=scopeItems;label.exchangeSlots.reset();label.exchangeSlotless.clear();}
                else {next.labels[scopeEditing.photo].exchangeSlots=scopeDraft;next.labels[scopeEditing.photo].exchangeSlotless=scopeSlotless;}
                if(CommitLibrary(std::move(next))) {SetStatus(fullOutfit?"Equipment selection saved. Applies on the next outfit change.":"Exchange slots saved. Applies on the next outfit change."); UI::CloseCurrentPopup();}
            }
        }
        if(UI::Button(Label(fullOutfit?"Use all saved equipment":"Restore original slot range"))) {
            auto next=library; next.labels[scopeEditing.photo].exchangeSlots.reset();next.labels[scopeEditing.photo].exchangeSlotless.clear();next.labels[scopeEditing.photo].outfitItems.reset();
            if(CommitLibrary(std::move(next))) UI::CloseCurrentPopup();
        }
        UI::EndChild();UI::Spacing();UI::Spacing();
        BeginEquipmentSection("Content section","Saved equipment",true);
        if(RenderPresetEditor(scopeEditing,libraryWritable && !editPending)) UI::CloseCurrentPopup();
        UI::EndChild();UI::Spacing();
        if(UI::CollapsingHeader(Label("Equipment help"))) {
        if(fullOutfit) UI::TextWrapped("%s",Tr("Replace the full outfit with checked equipment. Unchecked equipment stays saved but is not worn. Previous outfit items are removed."));
        else {
        UI::TextWrapped("%s",Tr("Slot selection changes only the exchange range. Equipment editing changes the saved outfit contents."));
        UI::TextWrapped(Tr("Items sharing a slot change together. Multi-slot items require all their slots."));
        UI::TextWrapped(Tr("Worn items overlapping selected slots are replaced as whole items, including their other slots."));
        }
        }
        UI::EndChild();
        UI::BeginDisabled(editPending);
        if(UI::Button(Label("Cancel"))) UI::CloseCurrentPopup();
        UI::EndDisabled();
        UI::EndPopup();
    }
    if(openTab) UI::OpenPopup("Edit category");
    if(UI::BeginPopup("Edit category")) {
        UI::InputText(Label("Tab name"),tabName,sizeof(tabName));
        if(UI::Button(Label("Save tab"))&&tabName[0]) {
            auto next=library;
            if(newCategory) {const auto id="tab-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()); next.categories.push_back({id,tabName});}
            else for(auto& c:next.categories) if(c.id==editCategory) c.name=tabName;
            if(CommitLibrary(std::move(next))) UI::CloseCurrentPopup();
        }
        if(!newCategory && UI::Button(Label("Remove tab (keep outfits)"))) {
            auto next=library; std::erase_if(next.categories,[&](const auto& c){return c.id==editCategory;});
            for(auto& [key,label]:next.labels) std::erase(label.categories,editCategory);
            if(CommitLibrary(std::move(next))) {category.clear(); UI::CloseCurrentPopup();}
        }
        if(UI::Button(Label("Cancel"))) UI::CloseCurrentPopup();
        UI::EndPopup();
    }
}




