// Renderer-only panel. No engine pointers, no game mutations.
const char* AutoPlaceLabel(unsigned n) {
    static constexpr std::array<const char*,27> labels{"Default","Town / City","Player home","Inn","Combat base","Night","Rain","Snow","Dungeon","Temple","Swimming","Fort Dawnguard","College of Winterhold","Dark Brotherhood sanctuaries","Jorrvaskr","Bards College","Ragged Flagon","Blue Palace","Combat: Melee","Combat: Magic","Combat: Archery","Combat: Unarmed","Sneaking","Smithing (test)","Alchemy (test)","Enchanting (test)","Shop"};
    return n<labels.size()?labels[n]:"Default";
}
// Presentation order only: saved slot IDs and policy priority remain unchanged.
constexpr std::array<unsigned,27> AutoDisplayOrder{0,1,2,3,26,9,8,4,18,19,20,21,22,10,5,6,7,11,12,13,14,15,16,17,23,24,25};
static_assert([] {std::array<bool,27> seen{};for(auto slot:AutoDisplayOrder){if(slot>=seen.size() || seen[slot]) return false;seen[slot]=true;}return true;}());
const char* AutoCardLabel(unsigned slot) {
    return slot==12?"Mage College":slot==13?"Dark Brotherhood":AutoPlaceLabel(slot);
}
// Snapshot settings for the current studio subject; global hotkeys remain player-only.
std::uint32_t autoUITarget{}; // renderer-only target snapshot for one gallery frame
std::uint32_t PanelFollowerID() {return autoUITarget;}
AutoOutfitSettings PanelAutoSettings() {
    if(const auto id=PanelFollowerID()) return GetFollowerAuto(id).settings;
    AutoOutfitSlots slots;{std::scoped_lock lock(autoMutex);slots=autoSlots;}
    return {slots,autoDelaySeconds.load(),autoColumns,ostimRecoverySeconds.load(),autoEnabled.load(),autoPaused.load(),autoCombatEnabled.load(),autoHideAssignButtons,autoSneakDelaySeconds.load(),autoCombatIntervalSeconds.load()};
}
bool SaveAutoOutfitPreferences(const AutoOutfitSlots& slots,int seconds,int columns,bool rearm,int recovery=-1,int sneak=-1,int combatInterval=-1,int hidden=-1) {
    if(const auto id=PanelFollowerID()) {
        if(!active || studioActorID.load()!=id) return false;
        auto settings=GetFollowerAuto(id).settings;
        settings.slots=slots;settings.delaySeconds=seconds;settings.columns=columns;
        if(recovery>=0) settings.ostimRecoverySeconds=recovery;
        if(sneak>=0) settings.sneakDelaySeconds=sneak;
        if(combatInterval>=0) settings.combatIntervalSeconds=combatInterval;
        if(hidden>=0) settings.hideAssignButtons=hidden!=0;
        return SetFollowerAuto(id,settings,rearm);
    }
    try {
        const bool nextHidden=hidden>=0?hidden!=0:autoHideAssignButtons;
        if(recovery<0) recovery=ostimRecoverySeconds.load();
        if(combatInterval<0) combatInterval=autoCombatIntervalSeconds.load();
        if(sneak<0) sneak=autoSneakDelaySeconds.load();
        WriteAutoOutfitSettings({slots,seconds,columns,recovery,autoEnabled.load(),autoPaused.load(),autoCombatEnabled.load(),nextHidden,sneak,combatInterval},autoFile);
        autoHideAssignButtons=nextHidden;
        autoCombatIntervalSeconds=combatInterval;ostimRecoverySeconds=recovery;autoSneakDelaySeconds=sneak;
        {std::scoped_lock lock(autoMutex); autoSlots=slots;}
        autoDelaySeconds=seconds; autoColumns=columns;
        if(rearm) ++autoRevision;
        SKSE::log::info("OG-AUTO settings delaySeconds={} columns={} rearm={}",seconds,columns,rearm);
        return true;
    }catch(const std::exception& e) {SetStatus(e.what()); return false;}
}
bool AssignAutoPhoto(unsigned slot,const std::string& photo) {
    if((!PanelFollowerID() && !autoSlotsWritable) || !libraryWritable || slot>=27 || (PanelFollowerID() && slot>=23 && slot<=25)) return false;
    if(!photo.empty()) {
        std::scoped_lock lock(presetsMutex);
        const auto it=std::find_if(presets.begin(),presets.end(),[&](const auto& p){return p.photo==photo;});
        const auto label=library.labels.find(photo);
        if(it==presets.end() || it->slotMask || it->accessories || it->exchangeSlots ||
           (label!=library.labels.end() && (label->second.deleted || label->second.exchangeSlots))) {
            SetStatus("Assign a full outfit without slot overrides"); return false;
        }
    }
    auto settings=PanelAutoSettings();settings.slots[slot]=photo;
    return SaveAutoOutfitPreferences(settings.slots,settings.delaySeconds,settings.columns,true);
}
bool SaveAutoOutfitToggles(bool enabled,bool paused,bool combat,bool playerOnly=false) {
    if(const auto id=playerOnly?0:PanelFollowerID()) {
        if(!active || studioActorID.load()!=id) return false;
        auto settings=GetFollowerAuto(id).settings;
        const bool rearm=enabled!=settings.enabled || combat!=settings.combatEnabled;
        settings.enabled=enabled;settings.paused=paused;settings.combatEnabled=combat;
        return SetFollowerAuto(id,settings,rearm);
    }
    try {
        AutoOutfitSlots slots; {std::scoped_lock lock(autoMutex);slots=autoSlots;}
        WriteAutoOutfitSettings({slots,autoDelaySeconds.load(),autoColumns,ostimRecoverySeconds.load(),enabled,paused,combat,autoHideAssignButtons,autoSneakDelaySeconds.load(),autoCombatIntervalSeconds.load()},autoFile);
        const bool rearm=enabled!=autoEnabled.load() || combat!=autoCombatEnabled.load();
        autoEnabled=enabled; autoPaused=paused; autoCombatEnabled=combat;
        if(rearm) ++autoRevision;
        SKSE::log::info("OG-AUTO toggles saved enabled={} paused={} combat={}",enabled,paused,combat);
        return true;
    }catch(const std::exception& e){SetStatus(e.what());return false;}
}
// Render-thread preference write, same path as the enable checkbox.
void ProcessAutoToggle() {
    if(!autoToggleRequested.exchange(false)) return;
    if(!autoSlotsWritable || !libraryWritable) {SetStatus("Automatic outfit settings are not writable.");return;}
    const bool enabled=!autoEnabled.load() || autoPaused.load();
    if(!SaveAutoOutfitToggles(enabled,false,autoCombatEnabled.load(),true)) return;
    const std::string message=Tr(enabled?"Automatic outfits: ON":"Automatic outfits: OFF");
    SetStatus(message);
    noticeCompact=true;
    noticeUntil=GetTickCount64()+2500;
    if(noticeWindow) noticeWindow->IsOpen=true;
}
void AutoSameLineIfFits(float width) {
    const float end=UI::GetWindowPos().x+UI::GetWindowSize().x-UI::GetStyle()->WindowPadding.x-UI::GetStyle()->ScrollbarSize;
    if(UI::GetItemRectMax().x+UI::GetStyle()->ItemSpacing.x+width<=end) UI::SameLine();
}
void RenderGalleryTitle(bool& opened) {
    autoUITarget=studioFollower.load()?studioActorID.load():0;
    const auto start=UI::GetCursorPos();
    const float buttonWidth=UI::CalcTextSize(Tr("Automatic outfits")).x+UI::GetStyle()->FramePadding.x*2.f;
    const std::string followerLabel=std::format("{} {}",Tr("Follower selection"),MaintainedFollowerCount());
    const float followerWidth=UI::CalcTextSize(followerLabel.c_str()).x+UI::GetStyle()->FramePadding.x*2.f;
    const float closeWidth=UI::GetFrameHeight();
    const float right=UI::GetContentRegionAvail().x;
    UI::AlignTextToFramePadding();UI::TextUnformatted("Outfit Gallery");
    const float x=std::max(UI::GetItemRectMax().x-UI::GetWindowPos().x+UI::GetStyle()->ItemSpacing.x,start.x+right-buttonWidth-followerWidth-closeWidth-UI::GetStyle()->ItemSpacing.x*2.f);
    UI::SetCursorPos({x,start.y});
    const bool selecting=followerTargeting.load();
    if(selecting) {
        UI::PushStyleColor(UI::ImGuiCol_Button,UI::ImVec4{.55f,.43f,.19f,1.f});
        UI::PushStyleColor(UI::ImGuiCol_ButtonHovered,UI::ImVec4{.65f,.52f,.24f,1.f});
    }
    if(UI::Button((followerLabel+"###followerSelection").c_str(),{followerWidth,closeWidth})) {followerTargeting=!selecting;settingsChangedAt=GetTickCount64();}
    if(selecting) UI::PopStyleColor(2);
    if(UI::IsItemHovered()) {
        UI::SetTooltip("%s\n%s\n%s",Tr("Select a follower as the outfit target."),Tr("Aim at a following companion and open the gallery."),Tr("The number counts enabled followers. Their outfits are maintained even when selection is OFF."));
    }
    UI::SameLine();
    const auto targetSettings=PanelAutoSettings();
    const bool enabled=targetSettings.enabled,paused=targetSettings.paused;
    if(enabled) {
        const UI::ImVec4 color=paused?UI::ImVec4{.22f,.36f,.48f,1.f}:UI::ImVec4{.55f,.43f,.19f,1.f};
        UI::PushStyleColor(UI::ImGuiCol_Button,color);
        UI::PushStyleColor(UI::ImGuiCol_ButtonHovered,paused?UI::ImVec4{.29f,.45f,.59f,1.f}:UI::ImVec4{.65f,.52f,.24f,1.f});
    }
    if(UI::Button(Label("Automatic outfits"),{buttonWidth,closeWidth})) {autoPanel=!autoPanel;autoPanelVisible=autoPanel;}
    if(enabled) UI::PopStyleColor(2);
    if(UI::IsItemHovered()) UI::SetTooltip("%s: %s\n%s",Tr("Automatic outfits"),Tr(!enabled?"Disabled":paused?"Paused":"Enabled"),Tr("Open or close settings. Automatic outfits keep running when this panel is closed."));
    UI::SameLine();if(UI::Button("X##closeGallery",{closeWidth,closeWidth})) opened=false;
    UI::Separator();
}
void RenderAutoOutfitHelp(bool follower=false) {
        UI::TextWrapped("%s",Tr("Shops use indoor locations tagged LocTypeStore. Home, inn, temple and dungeon take priority. An empty Shop slot uses Town inside towns, otherwise Default."));
        if(!follower) UI::TextWrapped("%s",Tr("Crafting trial: assign a full outfit to Smithing, Alchemy or Enchanting. Missing base items are generated before the crafting menu; carry your custom-enchanted items. Late or blocked attempts are skipped until the next use. Check the equipped outfit and its active effects."));
        UI::TextWrapped("%s",Tr("Sneaking uses its own delay in Options (0-5 seconds, default 2). Combat and swimming take priority; otherwise sneaking overrides weather and locations. Empty slots keep existing rules."));
        UI::TextWrapped("%s",Tr("Styles must stay stable for 1 second, or 2 seconds for Unarmed. Attack/casting and weapon draw transitions wait. The usual delay and configured combat interval still apply. Manual changes last until combat ends."));
        UI::TextWrapped("%s",Tr("Combat styles follow the right hand; bows use either hand. Unarmed requires both hands empty. Empty style slots use Combat. Leave all four empty for the original once-per-combat behavior."));
        UI::TextWrapped("%s",Tr("Outdoors: Snow > Rain > Night > location. Night is 20:00-06:00. Home, inn, temple and dungeon keep dedicated location rules."));
        UI::TextWrapped("%s",Tr("Empty temple/dungeon slots use Town only inside a town, otherwise Default. Overlapping locations: Home > Inn > Temple > Dungeon > Town."));
        UI::TextWrapped("%s",Tr("Player home or Inn takes priority over Town / City, then Default. Empty slots fall back; invalid assigned sets are skipped."));
        UI::TextWrapped("%s",Tr("Assigned swimming outfits override weather and locations. Combat has priority. Shallow water does not trigger swimming."));
        UI::TextWrapped("%s",Tr("Guild outfits apply indoors only. Empty guild slots use the usual location rules. College courtyards and the Midden are excluded."));
        UI::TextWrapped("%s",Tr("Missing inventory items are generated. Missing mod records stop the whole outfit change."));
        UI::TextWrapped("%s",Tr("Manual Gallery changes take priority until the selected condition changes. Pause before posing or using another outfit manager."));
}
void RenderAutoOutfitPanel(UI::ImVec2 screen) {
    const auto followerID=PanelFollowerID();
    const auto panelSettings=PanelAutoSettings();
    const bool panelEnabled{panelSettings.enabled},panelPaused{panelSettings.paused},panelCombat{panelSettings.combatEnabled};
    const int panelDelay{panelSettings.delaySeconds},panelCombatInterval{panelSettings.combatIntervalSeconds},panelSneakDelay{panelSettings.sneakDelaySeconds},panelRecovery{panelSettings.ostimRecoverySeconds};
    const int panelColumns=panelSettings.columns;
    bool panelHideAssign=panelSettings.hideAssignButtons;
    const bool panelWritable=followerID?true:Gallery::autoSlotsWritable;
    std::vector<unsigned> displayOrder;
    for(auto slot:AutoDisplayOrder) if(!followerID || slot<23 || slot>25) displayOrder.push_back(slot);

    UI::SetNextWindowPos({screen.x*.535f,screen.y*.015f},UI::ImGuiCond_Always);
    UI::SetNextWindowSize({screen.x*.455f,screen.y*.97f},UI::ImGuiCond_Always);
    bool panelOpen=autoPanel;
    if(UI::Begin(Label("Automatic outfits (test)"),&panelOpen,UI::ImGuiWindowFlags_NoResize|UI::ImGuiWindowFlags_NoMove|UI::ImGuiWindowFlags_NoCollapse)) {
        std::string targetName;
        {std::scoped_lock lock(statusMutex);targetName=studioActorName;}
        UI::Text("%s: %s",Tr("Target"),followerID?targetName.c_str():Tr("Player"));
        if(followerID) UI::TextWrapped("%s",Tr("Settings are saved when you save the game (up to two followers).\nEnabling automatic outfits disables fixed outfit maintenance."));
        bool enabled=panelEnabled, paused=panelPaused, combat=panelCombat;
        UI::BeginDisabled(!panelWritable || !libraryWritable);
        if(UI::Checkbox(Label("Enable automatic outfits"),&enabled)) SaveAutoOutfitToggles(enabled,paused,combat);
        const char* pauseText=panelPaused?"Resume":"Pause";
        AutoSameLineIfFits(UI::CalcTextSize(Tr(pauseText)).x+UI::GetStyle()->FramePadding.x*2.f);
        UI::BeginDisabled(!panelEnabled);
        if(UI::Button(Label(pauseText))) SaveAutoOutfitToggles(panelEnabled,!panelPaused,panelCombat);
        UI::EndDisabled();
        AutoSameLineIfFits(UI::CalcTextSize(Tr("Combat (experimental)")).x+UI::GetFrameHeight()+UI::GetStyle()->ItemInnerSpacing.x);
        if(UI::Checkbox(Label("Combat (experimental)"),&combat)) SaveAutoOutfitToggles(panelEnabled,panelPaused,combat);
        UI::EndDisabled();
        AutoSameLineIfFits(UI::GetFrameHeight());
        if(UI::Button("?##autoHelp")) UI::OpenPopup("Auto outfit help");
        UI::SetNextWindowSize({screen.x*.40f,screen.y*.60f},UI::ImGuiCond_Appearing);
        if(UI::BeginPopup("Auto outfit help")) {
            UI::TextWrapped("%s",Tr("Combat testing: armor and effects change during combat. Not verified across mod setups; use a test save."));
            UI::Separator();
            UI::TextWrapped("%s",Tr(followerID?"Follower settings belong to this game save. Closing the panel keeps automatic outfits running.":"Automatic outfit preferences are shared across saves. Closing this panel does not disable automatic outfits."));
            RenderAutoOutfitHelp(followerID!=0);
            UI::EndPopup();
        }
        std::string state,loc; AutoOutfitSlots slots; AutoEnvironmentDisplay environment;
        if(followerID) {const auto entry=GetFollowerAuto(followerID);state=entry.status;loc=entry.location;slots=entry.settings.slots;environment=entry.environment;}
        else {std::scoped_lock lock(autoMutex); state=autoState; loc=autoLocation; slots=autoSlots; environment=autoEnvironment;}
        int seconds=panelDelay;
        UI::BeginDisabled(!panelWritable || !libraryWritable);
        UI::SetNextItemWidth(std::max(90.f,UI::GetContentRegionAvail().x*.42f));
        if(UI::SliderInt(Label("Change delay (seconds)"),&seconds,0,10,"%d",UI::ImGuiSliderFlags_AlwaysClamp))
            SaveAutoOutfitPreferences(slots,seconds,panelColumns,false);
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("0 seconds: change on the next check (about once per second). The minimum attempt interval is 5 seconds outside combat; combat uses its configured interval."));
        UI::EndDisabled();
        if(UI::CollapsingHeader(Label("Registered automatic followers"))) {
            std::vector<std::pair<std::uint32_t,std::string>> registered;
            {std::scoped_lock lock(followerAutoMutex);for(const auto& [id,e]:followerAutos) registered.emplace_back(id,e.name);}
            for(const auto& [id,name]:registered) {
                UI::PushID(static_cast<int>(id));
                UI::TextUnformatted(name.empty()?Tr("Follower"):name.c_str());UI::SameLine();
                if(UI::Button(Label("Remove automatic settings"))) RemoveFollowerAuto(id);
                UI::PopID();
            }
        }
        if(UI::CollapsingHeader(Label("Options"))) {
            if(!followerID) RenderAutoHotkeys();
            else UI::TextWrapped("%s",Tr("The automatic outfit hotkey controls the player only."));
            UI::Separator();
            UI::BeginDisabled(!panelWritable || !libraryWritable);
        int combatInterval=panelCombatInterval;
        UI::SetNextItemWidth(std::max(90.f,UI::GetContentRegionAvail().x*.42f));
        if(UI::SliderInt(Label("Minimum combat switch interval (seconds)"),&combatInterval,2,10,"%d",UI::ImGuiSliderFlags_AlwaysClamp)) {
            SaveAutoOutfitPreferences(slots,panelDelay,panelColumns,false,-1,-1,combatInterval);
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Default: 5 seconds (recommended). Lower values respond faster to repeated weapon changes, but may increase outfit-change load or interactions with other mods. Stability and attack/casting waits still apply."));
        int sneak=panelSneakDelay;
        UI::SetNextItemWidth(std::max(90.f,UI::GetContentRegionAvail().x*.42f));
        if(UI::SliderInt(Label("Sneaking delay (seconds)"),&sneak,0,5,"%d",UI::ImGuiSliderFlags_AlwaysClamp))
            SaveAutoOutfitPreferences(slots,panelDelay,panelColumns,false,-1,sneak);
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Independent of the normal delay. 0 uses the next check (about once per second). The 5-second attempt interval and attack/casting waits still apply."));
        int recovery=panelRecovery;
        UI::SetNextItemWidth(std::max(90.f,UI::GetContentRegionAvail().x*.42f));
        if(UI::SliderInt(Label("OStim recovery delay (seconds)"),&recovery,5,30,"%d",UI::ImGuiSliderFlags_AlwaysClamp))
            SaveAutoOutfitPreferences(slots,seconds,panelColumns,false,recovery);
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Wait after confirmed scene exit, then evaluate current conditions. Changes apply to the next scene exit."));

            bool hide=panelHideAssign;
            if(UI::Checkbox(Label("Hide assignment buttons"),&hide)) {
                const bool previous=panelHideAssign;panelHideAssign=hide;
                if(!SaveAutoOutfitPreferences(slots,panelDelay,panelColumns,false,-1,-1,-1,hide?1:0)) panelHideAssign=previous;
            }
            if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Drag photos to assign outfits. Hover over an assigned photo to clear its assignment."));
            UI::EndDisabled();
        }
        UI::Text("%s: %s",Tr("Current location"),Tr(AutoPlaceLabel(environment.locationPlace)));
        AutoSameLineIfFits(UI::CalcTextSize(Tr("Details")).x+UI::GetStyle()->FramePadding.x*2.f);
        if(UI::Button(Label("Details"))) UI::OpenPopup("Auto outfit details");
        UI::SetNextWindowSize({screen.x*.38f,0},UI::ImGuiCond_Appearing);
        if(UI::BeginPopup("Auto outfit details")) {
        if(!loc.empty()) UI::TextWrapped("%s",loc.c_str());
        const auto& env=environment.environment;
        if(env.ready) {
            const int minutes=static_cast<int>(env.hour*60.f)%1440;
            UI::Text("%s / %02d:%02d / %s",Tr(env.flags.outside?"Outdoors":"Indoors"),minutes/60,minutes%60,Tr(env.flags.night?"Night":"Daytime"));
            UI::Text("%s: %s",Tr("Precipitation"),Tr(!env.flags.outside?"Not applicable indoors":env.flags.snow?(env.flags.rain?"Rain and snow":"Snow"):env.flags.rain?"Rain":"None"));
        } else UI::TextUnformatted(Tr("Waiting for environment data"));
        if(env.flags.swimming) UI::TextUnformatted(Tr("Swimming detected"));
        if(env.flags.sneaking) UI::TextUnformatted(Tr("Sneaking detected"));
        UI::Text("%s: %s",Tr("Selected outfit rule"),environment.selectedSlot<0?Tr("Not assigned"):Tr(AutoPlaceLabel(environment.selectedSlot)));
        UI::TextWrapped("%s: %s",Tr("Status"),Tr(state.c_str()));
        if(!followerID) UI::TextWrapped("%s",Tr(OStimStateLabel(ostimSceneState.load())));
            UI::EndPopup();
        }
        UI::Separator();
        const float buttonHeight=UI::GetFrameHeight();
        UI::BeginDisabled(!panelWritable || !libraryWritable);
        if(UI::Button("##panelColumns",{buttonHeight+20.f,buttonHeight}))
            SaveAutoOutfitPreferences(slots,panelDelay,panelColumns==5?3:panelColumns+1,false);
        DrawColumnCountIcon(buttonHeight);
        if(UI::IsItemHovered()) UI::SetTooltip(Tr("Columns: %d (click to cycle)"),panelColumns);
        UI::EndDisabled();
        UI::SameLine(); UI::Text(Tr("Columns: %d"),panelColumns);
        UI::Spacing();
        const auto start=UI::GetCursorScreenPos();
        const float gap=UI::GetStyle()->ItemSpacing.x*2.f;
        const float width=std::max(32.f,(UI::GetContentRegionAvail().x-gap*(panelColumns-1))/panelColumns);
        const float frameX=UI::GetStyle()->FramePadding.x, frameY=UI::GetStyle()->FramePadding.y;
        const float imageWidth=std::max(1.f,width-frameX*2.f);
        const float cardHeight=imageWidth*1.5f;
        float rowY=start.y, rowBottom=start.y, rowHeaderHeight=0.f;
        for(unsigned displayIndex=0;displayIndex<displayOrder.size();++displayIndex) {
            const unsigned n=displayOrder[displayIndex];
            const unsigned col=displayIndex%static_cast<unsigned>(panelColumns);
            if(col==0) {
                if(displayIndex) rowY=rowBottom+gap;
                rowHeaderHeight=UI::GetTextLineHeight();
                const auto end=std::min(static_cast<unsigned>(displayOrder.size()),displayIndex+static_cast<unsigned>(panelColumns));
                for(unsigned next=displayIndex;next<end;++next)
                    rowHeaderHeight=std::max(rowHeaderHeight,UI::CalcTextSize(Tr(AutoCardLabel(displayOrder[next])),nullptr,false,width).y);
            }
            UI::SetCursorScreenPos({start.x+col*(width+gap),rowY});
            UI::PushID(static_cast<int>(n)); UI::BeginGroup();
            const bool combatOff=!panelCombat && (n==4 || (n>=18 && n<=21));
            if(combatOff) UI::PushStyleVar(UI::ImGuiStyleVar_Alpha,UI::GetStyle()->Alpha*.45f);
            UI::PushTextWrapPos(UI::GetCursorPosX()+width);
            UI::TextWrapped("%s",Tr(AutoCardLabel(n)));
            UI::PopTextWrapPos();
            if(UI::IsItemHovered()) UI::SetTooltip("%s%s%s",Tr(n==4?"Used during combat when the matching weapon-style slot is empty or no style matches.":AutoPlaceLabel(n)),combatOff?"\n":"",combatOff?Tr("Combat switching is OFF. Assignments can still be edited."):"");
            UI::SetCursorScreenPos({start.x+col*(width+gap),rowY+rowHeaderHeight+UI::GetStyle()->ItemSpacing.y});
            const auto path=(std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/slots[n]).generic_string();
            const auto tex=slots[n].empty()?nullptr:LoadPortraitView(path);
            UI::SetNextItemAllowOverlap();
            if(tex) {
                auto* view=static_cast<ID3D11ShaderResourceView*>(tex);
                Microsoft::WRL::ComPtr<ID3D11Resource> resource; view->GetResource(resource.GetAddressOf());
                Microsoft::WRL::ComPtr<ID3D11Texture2D> image; D3D11_TEXTURE2D_DESC desc{};
                if(resource && SUCCEEDED(resource.As(&image))) image->GetDesc(&desc);
                const float sourceAspect=desc.Height?float(desc.Width)/desc.Height:1.f;
                const float targetAspect=imageWidth/cardHeight;
                UI::ImVec2 uv0{0,0},uv1{1,1};
                if(sourceAspect>targetAspect) {const float fraction=targetAspect/sourceAspect; uv0.x=(1.f-fraction)*.5f; uv1.x=1.f-uv0.x;}
                else {const float fraction=sourceAspect/targetAspect; uv0.y=(1.f-fraction)*.5f; uv1.y=1.f-uv0.y;}
                UI::ImageButton("##autoSlot",tex,{imageWidth,cardHeight},uv0,uv1);
            }
            else UI::Button(Tr("Drop outfit here"),{width,cardHeight+frameY*2.f});
            const auto photoMin=UI::GetItemRectMin(),photoMax=UI::GetItemRectMax();
            const bool photoHovered=UI::IsItemHovered(UI::ImGuiHoveredFlags_AllowWhenOverlappedByItem | UI::ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            if(UI::BeginDragDropTarget()) {
                if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_PHOTO")) {
                    if(payload->Data && payload->DataSize>1 && payload->DataSize<=241 && static_cast<const char*>(payload->Data)[payload->DataSize-1]=='\0')
                        AssignAutoPhoto(n,std::string(static_cast<const char*>(payload->Data),payload->DataSize-1));
                }
                UI::EndDragDropTarget();
            }
            std::string title=Tr("Not assigned");
            if(!slots[n].empty()) {
                title=Tr("Assigned preset not found");
                {std::scoped_lock lock(presetsMutex); for(const auto& p:presets) if(p.photo==slots[n]) {title=p.name; break;}}
                if(auto label=library.labels.find(slots[n]); label!=library.labels.end()) {
                    if(label->second.deleted) title=Tr("Assigned preset is in Trash");
                    else if(!label->second.name.empty()) title=label->second.name;
                }
            }
            if(photoHovered && !UI::GetDragDropPayload()) {
                const auto cursor=UI::GetCursorPos();
                const float size=std::min(UI::GetFrameHeight(),width*.32f);
                bool clearHovered=false;
                if(!slots[n].empty()) {
                    UI::SetCursorScreenPos({photoMax.x-size-3.f,photoMin.y+3.f});
                    UI::BeginDisabled(!panelWritable || !libraryWritable);
                    if(UI::Button("X##clearAssignment",{size,size})) AssignAutoPhoto(n,"");
                    clearHovered=UI::IsItemHovered();
                    if(clearHovered) UI::SetTooltip("%s",Tr("Clear assignment"));
                    UI::EndDisabled();
                    UI::SetCursorPos(cursor);
                }
                if(!clearHovered) UI::SetTooltip("%s%s%s",title.c_str(),combatOff?"\n":"",combatOff?Tr("Combat switching is OFF. Assignments can still be edited."):"");
            }
            if(!panelHideAssign) {
            UI::BeginDisabled(!panelWritable || !libraryWritable || autoSelectedPhoto.empty());
            const bool compactAssign=UI::CalcTextSize(Tr("Assign selected")).x+frameX*2.f>width;
            if(UI::Button(Label(compactAssign?"Assign":"Assign selected"),{width,0})) AssignAutoPhoto(n,autoSelectedPhoto);
            if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Assign selected"));
            UI::EndDisabled();
            }
            if(combatOff) UI::PopStyleVar();
            UI::EndGroup();
            rowBottom=std::max(rowBottom,UI::GetItemRectMax().y);
            UI::PopID();
        }
        UI::SetCursorScreenPos({start.x,rowBottom+gap});
        UI::Separator();
        if(!panelWritable) UI::TextWrapped("%s",Tr("Auto outfit settings could not be read. Editing is disabled."));
    }
    UI::End();
    if(!panelOpen) {autoPanel=false;autoPanelVisible=false;}
}
