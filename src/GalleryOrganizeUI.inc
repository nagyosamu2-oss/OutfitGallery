// Render-thread-owned transaction. Equipment and preset files are never changed here.
struct OrganizerState {
    bool active{},openPopup{},destination{},toolbar{},targetBar{};
    int action{},focus{},targetFocus{};
    unsigned revision{},presetsRevision{},opening{};
    Library draft;
    Variations::Interaction variation;
    UI::ImVec2 variationMin{},variationMax{};
    std::vector<Preset> snapshot;
    std::set<std::string> selection;
    std::string category,anchor,context,error,filter;
    std::string representativeDragPhoto;
    std::set<std::string> representativeDragSelection;
    bool representativeDragging{};
    std::string registrationTab;
    unsigned registrationAdded{},registrationExisting{},registrationRemoved{};
    char search[160]{};
    std::vector<std::string> loaded;
    std::optional<std::string> disk;
};
OrganizerState organizer;
bool organizerFrame{};
std::vector<std::string> organizerRetiredTextures;
void DrainOrganizerTextures() {
    for(const auto& path:organizerRetiredTextures) DisposePortraitView(path);
    organizerRetiredTextures.clear();
}
bool organizerShowNames=true;
const std::filesystem::path organizerNamesSettings="Data/SKSE/Plugins/OutfitGallery/OrganizerNames.json";
void SaveOrganizerNames() {
    try {std::filesystem::create_directories(organizerNamesSettings.parent_path());
        std::ofstream out(organizerNamesSettings);out.exceptions(std::ios::badbit|std::ios::failbit);
        out<<(organizerShowNames?"true":"false");out.close();
    }catch(const std::exception& e){organizer.error=e.what();}
}
int organizerColumns=8; // Independent of ordinary gallery settings.
const std::filesystem::path organizerSettings="Data/SKSE/Plugins/OutfitGallery/Organizer.json";
std::optional<std::string> OrganizerDisk() {
    if(!std::filesystem::exists(libraryFile)) return std::nullopt;
    std::ifstream in(libraryFile,std::ios::binary);
    if(!in) throw std::runtime_error("Cannot read Library.json");
    std::string bytes((std::istreambuf_iterator<char>(in)),{});
    if(in.bad()) throw std::runtime_error("Cannot read Library.json");
    return bytes;
}
void EndOrganizer() {
    // ImGui draw commands from the closing frame still refer to these views.
    organizerRetiredTextures.insert(organizerRetiredTextures.end(),organizer.loaded.begin(),organizer.loaded.end());
    organizer={}; organizerInputActive=false; SetMissingCheckMode(false);
}
void StartOrganizer(const std::string& category) {
    EndOrganizer();
    try {
        organizer.disk=OrganizerDisk();
        organizer.draft=library; organizer.revision=libraryRevision;
        {std::scoped_lock lock(presetsMutex);organizer.snapshot=presets;organizer.presetsRevision=presetRevision.load();}
        std::map<std::string,int> ranks;
        for(std::size_t i=0;i<library.order.size();++i) ranks[library.order[i]]=static_cast<int>(i);
        auto rank=[&](const auto& p){auto it=ranks.find(p.photo);return it==ranks.end()?-1:it->second;};
        std::stable_sort(organizer.snapshot.begin(),organizer.snapshot.end(),[&](const auto& a,const auto& b){return rank(a)<rank(b);});
        std::set<std::string> ids; organizer.draft.order.clear();
        for(const auto& p:organizer.snapshot) if(ids.insert(p.photo).second) organizer.draft.order.push_back(p.photo);
        for(const auto& id:library.order) if(ids.insert(id).second) organizer.draft.order.push_back(id);
        organizer.category=category;organizer.opening=studioOpenSerial.load();
        static bool settingsLoaded=false;
        if(!settingsLoaded) {
            settingsLoaded=true;
            try {std::ifstream in(organizerSettings);int n;if(in>>n && n>=4 && n<=30)organizerColumns=n;}catch(...){}
            try {std::ifstream in(organizerNamesSettings);std::string value;if(in>>value && (value=="true" || value=="false"))organizerShowNames=value=="true";}catch(...){}
        }
        organizer.active=true;organizer.openPopup=true;organizerInputActive=true;captureAllowed=false;helpOpen=false;
    } catch(const std::exception& e) {EndOrganizer();SetStatus(e.what());}
}
void RenderOrganizer() {
    organizerFrame=true;captureAllowed=false;
    if(!organizer.active) return;
    if(organizer.opening!=studioOpenSerial.load()) {EndOrganizer();return;}
    ReceiveMissingCheck();
    const auto display=UI::GetIO()->DisplaySize, fb=UI::GetIO()->DisplayFramebufferScale;
    const auto dims=ResolveLayoutSize({display.x,display.y},{fb.x,fb.y},ReadLayoutSurface());
    if(organizer.openPopup) {UI::OpenPopup(Label("Gallery organizer"));organizer.openPopup=false;}
    UI::SetNextWindowPos({dims.width*.025f,dims.height*.035f},UI::ImGuiCond_Always);
    UI::SetNextWindowSize({dims.width*.95f,dims.height*.93f},UI::ImGuiCond_Always);
    bool open=true;
    if(!UI::BeginPopupModal(Label("Gallery organizer"),&open,UI::ImGuiWindowFlags_NoSavedSettings|UI::ImGuiWindowFlags_NoResize|UI::ImGuiWindowFlags_NoMove|UI::ImGuiWindowFlags_NoNavInputs)) {
        if(!open || !UI::IsPopupOpen(Label("Gallery organizer"))) EndOrganizer();
        return;
    }
    bool close=!open,save=false;
    const bool conflict=organizer.revision!=libraryRevision || organizer.presetsRevision!=presetRevision.load();
    if(conflict) UI::TextWrapped("%s",Tr("Gallery changed while organizing. Cancel and reopen before saving."));
    const auto tabs=CollectionTabs(library,true);
    auto tabIt=std::find_if(tabs.begin(),tabs.end(),[&](const auto& c){return c.id==organizer.category;});
    int tab=tabIt==tabs.end()?0:static_cast<int>(tabIt-tabs.begin());
    UI::SetNextItemWidth(UI::GetContentRegionAvail().x*.20f);
    if(UI::BeginCombo(Label("Organizer category"),CategoryTitle(tabs[tab]).c_str())) {
        for(const auto& c:tabs) if(UI::Selectable((CategoryTitle(c)+"##org"+c.id).c_str(),c.id==organizer.category)) organizer.category=c.id;
        UI::EndCombo();
    }
    UI::SameLine();UI::SetNextItemWidth(UI::GetContentRegionAvail().x*.35f);
    UI::InputText(Label("Search outfits"),organizer.search,sizeof(organizer.search));
    UI::SameLine();UI::SetNextItemWidth(UI::GetContentRegionAvail().x*.55f);
    if(UI::SliderInt(Label("Organizer columns"),&organizerColumns,4,30)) {
        try {std::filesystem::create_directories(organizerSettings.parent_path());std::ofstream out(organizerSettings);out.exceptions(std::ios::badbit|std::ios::failbit);out<<organizerColumns;out.close();}catch(const std::exception& e){organizer.error=e.what();}
    }
    std::vector<Preset> cards=organizer.snapshot;
    std::map<std::string,std::size_t> ranks;
    for(std::size_t i=0;i<organizer.draft.order.size();++i)ranks[organizer.draft.order[i]]=i;
    std::stable_sort(cards.begin(),cards.end(),[&](const auto& a,const auto& b){return ranks.at(a.photo)<ranks.at(b.photo);});
    for(auto& p:cards) {auto it=organizer.draft.labels.find(p.photo);if(it!=organizer.draft.labels.end() && !it->second.name.empty())p.name=it->second.name;}
    std::erase_if(cards,[&](const auto& p){
        auto it=organizer.draft.labels.find(p.photo);
        const bool deleted=it!=organizer.draft.labels.end() && it->second.deleted;
        return deleted!=(organizer.category=="#trash") || !CollectionFits(p.slotMask,organizer.category,p.accessories,organizer.draft) ||
            (!organizer.category.empty() && !organizer.category.starts_with("#") && (it==organizer.draft.labels.end() || std::find(it->second.categories.begin(),it->second.categories.end(),organizer.category)==it->second.categories.end())) ||
            (organizer.search[0] && p.name.find(organizer.search)==std::string::npos);
    });
    const auto filter=organizer.category+"|"+organizer.search;
    const bool reset=filter!=organizer.filter;
    if(reset) {organizer.representativeDragPhoto.clear();organizer.representativeDragSelection.clear();organizer.representativeDragging=false;organizer.filter=filter;organizer.selection.clear();organizer.anchor.clear();organizer.destination=false;organizer.focus=0;organizer.registrationTab.clear();}
    if(missingCheckMode && !missingCheckPending && missingCheckError.empty())MissingPresetsFirst(cards,missingResults);
    const auto allCards=cards;
    std::vector<std::string> view;for(const auto& p:allCards)view.push_back(p.photo);
    const auto groups=(!missingCheckMode && organizer.category!="#trash")?Variations::Collapse(cards,organizer.draft):std::map<std::string,std::vector<Preset>>{};
    if(!organizer.variation.Reconcile(groups) || reset) {
        if(UI::BeginPopup("Organizer variations")){UI::CloseCurrentPopup();UI::EndPopup();}
        organizer.variation={};
    }
    bool variationHover=false,variationAnchor=false,openVariation=false;
    const auto now=GetTickCount64();
    const bool padOK=!UI::IsPopupOpen("Organizer variations") && !UI::GetIO()->WantTextInput && !UI::IsAnyItemActive() && !UI::IsPopupOpen("Organizer details");
    auto pad=[&](auto key,bool repeat=false){return padOK && PadPressed(key,repeat);};
    const bool confirm=pad(UI::ImGuiKey_GamepadFaceDown);
    const bool left=pad(UI::ImGuiKey_GamepadDpadLeft,true)||pad(UI::ImGuiKey_GamepadLStickLeft,true);
    const bool right=pad(UI::ImGuiKey_GamepadDpadRight,true)||pad(UI::ImGuiKey_GamepadLStickRight,true);
    const bool up=pad(UI::ImGuiKey_GamepadDpadUp,true)||pad(UI::ImGuiKey_GamepadLStickUp,true);
    const bool down=pad(UI::ImGuiKey_GamepadDpadDown,true)||pad(UI::ImGuiKey_GamepadLStickDown,true);
    bool scrolled=false;
    if(organizer.toolbar) {
        if(left||right)organizer.action=(organizer.action+(right?1:9))%10;
        if(down){organizer.toolbar=false;organizer.targetBar=true;}
    } else if(organizer.targetBar) {
        const auto count=static_cast<int>(organizer.draft.categories.size());
        if(count && (left||right))organizer.targetFocus=(organizer.targetFocus+(right?1:count-1))%count;
        if(up){organizer.targetBar=false;organizer.toolbar=true;}
        if(down){organizer.targetBar=false;scrolled=true;}
    } else {
        if(up && organizer.focus<organizerColumns)organizer.targetBar=true;
        else if(up){organizer.focus-=organizerColumns;scrolled=true;}
        if(down){organizer.focus+=organizerColumns;scrolled=true;}
        if(left){--organizer.focus;scrolled=true;}if(right){++organizer.focus;scrolled=true;}
    }
    organizer.focus=std::clamp(organizer.focus,0,std::max(0,static_cast<int>(cards.size())-1));
    if(pad(UI::ImGuiKey_GamepadL1)||pad(UI::ImGuiKey_GamepadR1)) organizer.category=tabs[(tab+(pad(UI::ImGuiKey_GamepadR1)?1:static_cast<int>(tabs.size())-1))%tabs.size()].id;
    if(!UI::IsPopupOpen("Organizer details") && !UI::IsPopupOpen("Organizer variations") && (pad(UI::ImGuiKey_GamepadFaceRight)||UI::IsKeyPressed(UI::ImGuiKey_Escape,false))) {
        if(organizer.destination)organizer.destination=false;else close=true;
    }
    auto button=[&](const char* label,int action,bool enabled=true) {
        UI::BeginDisabled(!enabled);
        bool clicked=UI::Button(Label(label));
        if(organizer.toolbar && organizer.action==action)UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
        UI::EndDisabled();return enabled && (clicked || (organizer.toolbar && organizer.action==action && confirm));
    };
    auto moveSelection=[&](const std::string& target=std::string{},bool after=false) {
        if(Organize::Move(organizer.draft.order,view,organizer.selection,target,after)) {
            organizer.selection.clear();organizer.anchor.clear();organizer.destination=false;organizer.registrationTab.clear();
        }
    };
    std::vector<std::string> trashTargets,categoryRemovals;
    const bool canTrash=!conflict && libraryWritable;
    auto trashLabel=[&](std::size_t count) {
        return std::string(Tr(organizer.category=="#trash"?"Restore from Trash":"Move to Trash"))+" ("+std::to_string(count)+")";
    };
    const bool canMove=!missingCheckMode && !organizer.selection.empty() && !conflict;
    if(button("Select visible",0)){organizer.selection.insert(view.begin(),view.end());organizer.registrationTab.clear();}UI::SameLine();
    if(button("Clear selection",1)){organizer.selection.clear();organizer.destination=false;organizer.registrationTab.clear();}UI::SameLine();
    if(button("Move to first",2,canMove))moveSelection();UI::SameLine();
    if(button("Move to last",3,canMove))moveSelection({},true);UI::SameLine();
    if(button("Choose destination",4,canMove))organizer.destination=!organizer.destination;UI::SameLine();
    const auto selectedTrash=Organize::TrashTargets(view,organizer.selection);
    UI::BeginDisabled(!canTrash || selectedTrash.empty());
    const float trashSize=UI::GetFrameHeight();
    const bool trashClick=organizer.category=="#trash"?
        UI::Button(Label("Restore from Trash")):UI::Button("##OrganizerTrash",{trashSize,trashSize});
    const auto trashMin=UI::GetItemRectMin(),trashMax=UI::GetItemRectMax();
    auto* trashDraw=UI::GetWindowDrawList();
    if(organizer.category!="#trash") {
        const float u=trashSize/24.f,x=trashMin.x,y=trashMin.y;
        const auto color=canTrash && !selectedTrash.empty()?0xFFE0E0E0:0xFF707070;
        UI::ImDrawListManager::AddRect(trashDraw,{x+7*u,y+8*u},{x+17*u,y+20*u},color,0,0,2.f);
        UI::ImDrawListManager::AddLine(trashDraw,{x+5*u,y+6*u},{x+19*u,y+6*u},color,2.f);
        UI::ImDrawListManager::AddLine(trashDraw,{x+9*u,y+3*u},{x+15*u,y+3*u},color,2.f);
        for(int k:{10,14})UI::ImDrawListManager::AddLine(trashDraw,{x+k*u,y+10*u},{x+k*u,y+18*u},color,1.5f);
    }
    if(organizer.toolbar && organizer.action==5)UI::ImDrawListManager::AddRect(trashDraw,trashMin,trashMax,0xFF30C8FF,0,0,3.f);
    UI::EndDisabled();
    if(UI::IsItemHovered())UI::SetTooltip("%s",trashLabel(selectedTrash.size()).c_str());
    if(canTrash && !selectedTrash.empty() && (trashClick || (organizer.toolbar && organizer.action==5 && confirm)))trashTargets=selectedTrash;
    UI::SameLine();
    UI::Text(Tr("Selected: %u / Visible: %u"),static_cast<unsigned>(organizer.selection.size()),static_cast<unsigned>(cards.size()));
    UI::SameLine(0,UI::GetStyle()->ItemSpacing.x*4.f);
    bool checking=missingCheckMode;
    if(UI::Checkbox(Label("Check missing gear"),&checking)) {SetMissingCheckMode(checking);organizer.destination=false;}
    if(organizer.toolbar && organizer.action==6)UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
    if(organizer.toolbar && organizer.action==6 && confirm){SetMissingCheckMode(!missingCheckMode);organizer.destination=false;}
    if(UI::IsItemHovered())UI::SetTooltip("%s",Tr("Marks outfits with missing parts, for example after removing a mod, with an X. Right-click a photo for details."));
    UI::SameLine(0,UI::GetStyle()->ItemSpacing.x*4.f);
    bool namesChanged=UI::Checkbox(Label("Show names in organizer"),&organizerShowNames);
    if(organizer.toolbar && organizer.action==7) {
        UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
        if(confirm){organizerShowNames=!organizerShowNames;namesChanged=true;}
    }
    if(namesChanged)SaveOrganizerNames();

    const float variationButtonsWidth=UI::CalcTextSize(Tr("Group variations")).x+UI::CalcTextSize(Tr("Ungroup variations")).x+
        UI::GetStyle()->FramePadding.x*4+UI::GetStyle()->ItemSpacing.x;
    UI::SameLine();
    const float variationRight=UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x-variationButtonsWidth;
    UI::SetCursorPosX(std::max(UI::GetCursorPosX(),variationRight));
    UI::BeginDisabled(conflict || !libraryWritable || organizer.category=="#trash" || missingCheckMode);
    auto groupSelection=[&](const std::set<std::string>& targets) {
        if(Variations::GroupAndClearSelection(organizer.draft,allCards,targets,organizer.selection)) {
            organizer.anchor.clear();organizer.registrationTab.clear();
        }
    };
    auto ungroupSelection=[&](const std::set<std::string>& targets) {
        Variations::UngroupAndClearSelection(organizer.draft,targets,organizer.selection);
        organizer.anchor.clear();organizer.registrationTab.clear();
    };
    UI::BeginDisabled(!Variations::CanGroup(allCards,organizer.selection));
    if(UI::Button(Label("Group variations"))) groupSelection(organizer.selection);
    UI::EndDisabled();
    if(UI::IsItemHovered(UI::ImGuiHoveredFlags_AllowWhenDisabled)) UI::SetTooltip("%s",Tr("Select matching outfit types. The first photo represents the group."));
    UI::SameLine();
    bool groupedSelection=false;
    for(const auto& group:organizer.draft.variations)for(const auto& photo:group)if(organizer.selection.contains(photo))groupedSelection=true;
    UI::BeginDisabled(!groupedSelection);
    if(UI::Button(Label("Ungroup variations"))) ungroupSelection(organizer.selection);
    UI::EndDisabled();
    if(UI::IsItemHovered(UI::ImGuiHoveredFlags_AllowWhenDisabled)) UI::SetTooltip("%s",Tr("Select matching outfit types. The first photo represents the group."));
    UI::EndDisabled();

    // Registration edits the same draft as ordering/trash. Only Save commits it.
    const auto outfitTargets=Organize::CategoryTargets(organizer.draft,allCards,organizer.selection);
    const auto accessoryTargets=Organize::CategoryTargets(organizer.draft,allCards,organizer.selection,true);
    UI::AlignTextToFramePadding();UI::TextUnformatted(Tr("Register to"));UI::SameLine();
    const auto* stripStyle=UI::GetStyle();
    const auto targetWidth=[&](const Category& c){return std::max(UiPx(60.f),UI::CalcTextSize(CategoryTitle(c).c_str()).x+stripStyle->FramePadding.x*2);};
    float targetTotal=0;
    for(const auto& c:organizer.draft.categories)targetTotal+=targetWidth(c)+stripStyle->ItemSpacing.x;
    const bool targetOverflow=targetTotal>UI::GetContentRegionAvail().x-stripStyle->WindowPadding.x*2;
    const float targetHeight=UI::GetFrameHeight()+stripStyle->WindowPadding.y*2+(targetOverflow?stripStyle->ScrollbarSize:0);
    const auto* drag=UI::GetDragDropPayload();
    const bool dragging=drag && std::strcmp(drag->DataType,"OUTFIT_ORGANIZE")==0;
    UI::BeginChild("Organizer registration tabs",{0,targetHeight},0,UI::ImGuiWindowFlags_HorizontalScrollbar|UI::ImGuiWindowFlags_NoNavInputs);
    organizer.targetFocus=std::clamp(organizer.targetFocus,0,std::max(0,static_cast<int>(organizer.draft.categories.size())-1));
    for(std::size_t n=0;n<organizer.draft.categories.size();++n) {
        if(n)UI::SameLine();
        const auto& c=organizer.draft.categories[n];
        const auto& registrationTargets=c.accessories?accessoryTargets:outfitTargets;
        const bool canRegister=!conflict && libraryWritable && !registrationTargets.empty();
        const auto title=CategoryTitle(c);
        const bool focused=organizer.targetBar && organizer.targetFocus==static_cast<int>(n);
        UI::BeginDisabled(!canRegister);
        UI::PushStyleColor(UI::ImGuiCol_ButtonHovered,{.65f,.52f,.24f,1.f});
        UI::PushStyleColor(UI::ImGuiCol_ButtonActive,{.72f,.57f,.25f,1.f});
        const bool clicked=UI::Button((title+"###org-register-"+c.id).c_str(),{targetWidth(c),UI::GetFrameHeight()});
        UI::PopStyleColor(2);
        const auto a=UI::GetItemRectMin(),b=UI::GetItemRectMax();
        bool delivered=false,preview=false;
        if(canRegister && UI::BeginDragDropTarget()) {
            if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_ORGANIZE",UI::ImGuiDragDropFlags_AcceptBeforeDelivery|UI::ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                preview=true;delivered=payload->Delivery;
            }
            UI::EndDragDropTarget();
        }
        UI::EndDisabled();
        if(preview || focused) {
            auto* draw=UI::GetWindowDrawList();
            if(preview)UI::ImDrawListManager::AddRectFilled(draw,a,b,0x4070BFFF,0,0);
            UI::ImDrawListManager::AddRect(draw,a,b,0xFF30C8FF,0,0,2.f);
        }
        if(focused && (left||right||up||down))UI::SetScrollHereX(.5f);
        if(canRegister && (clicked || delivered || (focused && confirm))) {
            organizer.registrationRemoved=0;
            organizer.registrationAdded=static_cast<unsigned>(Organize::AddToCategory(organizer.draft,registrationTargets,c.id));
            organizer.registrationExisting=static_cast<unsigned>(registrationTargets.size())-organizer.registrationAdded;
            organizer.registrationTab=title;organizer.destination=false;
        }
        if(!dragging && UI::IsItemHovered(UI::ImGuiHoveredFlags_AllowWhenDisabled)) {
            if(conflict)UI::SetTooltip("%s",Tr("Gallery changed while organizing. Cancel and reopen before saving."));
            else if(!libraryWritable)UI::SetTooltip("%s",Tr("The gallery library is not writable."));
            else if(registrationTargets.empty())UI::SetTooltip("%s",Tr(c.accessories?"Select accessory sets to register. Restore photos from Trash first.":"Select full outfits to register. Restore photos from Trash first."));
            else UI::SetTooltip(Tr("Drop or click to add %u selected outfits to this tab. Existing memberships are kept. Save and close to apply."),static_cast<unsigned>(registrationTargets.size()));
        }
    }
    if(organizer.draft.categories.empty())UI::TextUnformatted(Tr("Create a category tab in the gallery options first."));
    // Keep distant tabs reachable without releasing a multi-photo drag.
    if(dragging && targetOverflow) {
        const auto pos=UI::GetWindowPos(),size=UI::GetWindowSize(),mouse=UI::GetIO()->MousePos;
        const float edge=std::min(UiPx(32.f),size.x*.2f);
        if(mouse.y>=pos.y && mouse.y<pos.y+size.y-stripStyle->ScrollbarSize && mouse.x>=pos.x && mouse.x<pos.x+size.x) {
            const float direction=mouse.x<pos.x+edge?-1.f:mouse.x>pos.x+size.x-edge?1.f:0.f;
            if(direction)UI::SetScrollX(std::clamp(UI::GetScrollX()+direction*UiPx(420.f)*UI::GetIO()->DeltaTime,0.f,UI::GetScrollMaxX()));
        }
    }
    UI::EndChild();
    if(!organizer.registrationTab.empty()) {
        if(organizer.registrationRemoved)UI::TextWrapped(Tr("%s: removed %u from this tab. Photos are kept. Applied when saved."),organizer.registrationTab.c_str(),organizer.registrationRemoved);
        else UI::TextWrapped(Tr("%s: added %u; already registered %u. Applied when saved."),organizer.registrationTab.c_str(),organizer.registrationAdded,organizer.registrationExisting);
    }
    if(missingCheckMode) {
        UI::TextWrapped("%s",Tr("Turn off missing gear checking to reorder. Your draft and selection are kept."));
        if(missingCheckPending)UI::TextUnformatted(Tr("Missing-data check mode: checking..."));
        else if(!missingCheckError.empty())UI::TextWrapped("%s",missingCheckError.c_str());
        else {auto counts=CountMissingPresets(organizer.snapshot,missingResults,[](const std::string& p){auto it=organizer.draft.labels.find(p);return it!=organizer.draft.labels.end()&&it->second.deleted;});UI::Text(Tr("Missing-data check mode: %u sets / %u in Trash"),counts.active,counts.trash);}
    } else if(organizer.destination) UI::TextWrapped("%s",Tr("Choose a photo or its left gap. Selected photos are inserted before the shaded photos, which follow them to the right. Escape/B cancels."));
    else if(organizer.registrationTab.empty())UI::TextWrapped("%s",Tr("Click to select; Shift-click for a range. Drag photos to reorder or onto a registration tab. Pad: up from photos for tabs, up again for actions; A to register; LB/RB to filter."));
    if(!organizer.error.empty())UI::TextWrapped("%s",organizer.error.c_str());
    const float footer=UI::GetFrameHeightWithSpacing()*2.f;
    UI::BeginChild("Organizer photos",{0,std::max(80.f,UI::GetContentRegionAvail().y-footer)},0,UI::ImGuiWindowFlags_NoNavInputs|UI::ImGuiWindowFlags_AlwaysVerticalScrollbar);
    const float gap=UI::GetStyle()->ItemSpacing.x+8.f;
    const float w=std::max(24.f,(UI::GetContentRegionAvail().x-gap*organizerColumns)/organizerColumns);
    const float h=w*1.42f,stepX=w+gap,stepY=h+(organizerShowNames?UI::GetTextLineHeightWithSpacing():0.f)+16.f;
    const int rows=(static_cast<int>(cards.size())+organizerColumns-1)/organizerColumns;
    float scroll=reset?0:UI::GetScrollY();const float viewport=std::max(1.f,UI::GetWindowHeight()-16.f);
    if(reset)UI::SetScrollY(0);
    if(scrolled) {const float y=organizer.focus/organizerColumns*stepY;if(y<scroll)scroll=y;else if(y+stepY>scroll+viewport)scroll=y+stepY-viewport;UI::SetScrollY(std::max(0.f,scroll));}
    const int start=std::clamp(static_cast<int>(scroll/stepY)-1,0,rows),finish=std::min(rows,start+static_cast<int>(viewport/stepY)+4);
    const float origin=UI::GetCursorPosY();std::vector<std::string> visible;
    std::string moveTarget,previewTarget;
    struct PreviewRect {int index;UI::ImVec2 a,b;};
    std::vector<PreviewRect> previewRects;
    if(organizer.destination && !organizer.toolbar && !organizer.targetBar && !cards.empty() && !organizer.selection.contains(cards[organizer.focus].photo))previewTarget=cards[organizer.focus].photo;
    auto renderDetails=[&](const char* popupID) {
    if(UI::BeginPopup(popupID)) {
        const auto found=std::find_if(allCards.begin(),allCards.end(),[](const auto& p){return p.photo==organizer.context;});
        if(found!=allCards.end())UI::TextWrapped("%s",found->name.c_str());
        if(missingCheckMode && !missingCheckPending) {
            auto it=missingResults.find(organizer.context);
            if(it!=missingResults.end())for(const auto& item:it->second){UI::TextWrapped("%s",item.name.c_str());UI::TextWrapped("%s / %06X",item.plugin.c_str(),item.localID);}
        }
        const auto targets=Organize::TrashTargets(view,organizer.selection,organizer.context);
        const std::set<std::string> contextSelection(targets.begin(),targets.end());
        bool groupedContext=false;
        for(const auto& group:organizer.draft.variations)for(const auto& photo:group)if(contextSelection.contains(photo))groupedContext=true;
        UI::BeginDisabled(!canTrash || missingCheckMode || organizer.category=="#trash");
        UI::BeginDisabled(!Variations::CanGroup(allCards,contextSelection));
        if(UI::Button(Label("Group variations"))) {groupSelection(contextSelection);UI::CloseCurrentPopup();}
        UI::EndDisabled();
        UI::BeginDisabled(!groupedContext);
        if(UI::Button(Label("Ungroup variations"))) {ungroupSelection(contextSelection);UI::CloseCurrentPopup();}
        UI::EndDisabled();UI::EndDisabled();UI::Separator();
        if(Organize::IsCategory(organizer.draft,organizer.category)) {
            UI::BeginDisabled(!canTrash || targets.empty());
            if(UI::Button((std::string(Tr("Remove from this tab"))+" ("+std::to_string(targets.size())+")##ContextUnregister").c_str())) {
                categoryRemovals=targets;UI::CloseCurrentPopup();
            }
            UI::EndDisabled();
            if(UI::IsItemHovered())UI::SetTooltip("%s",Tr("Removes only this tab's membership. Photos and other tabs are kept. Applied when saved."));
            UI::Separator();
        }
        UI::BeginDisabled(!canTrash || targets.empty());
        if(UI::Button((trashLabel(targets.size())+"##ContextTrash").c_str())) {
            trashTargets=targets;UI::CloseCurrentPopup();
        }
        UI::EndDisabled();
        UI::TextWrapped("%s",Tr("Trash changes are also applied only when saved."));UI::EndPopup();
    }
    };
    bool details=false,variationDetails=false;
    auto choose=[&](const std::string& photo,bool range) {
        organizer.registrationTab.clear();
        if(organizer.destination && !missingCheckMode) {moveTarget=photo;return;}
        if(range) {
            Organize::SelectRange(organizer.selection,view,organizer.anchor,photo);
            for(const auto& [representative,choices]:groups)if(organizer.selection.contains(representative))
                for(const auto& member:choices)organizer.selection.insert(member.photo);
        }
        else {Variations::ToggleSelection(organizer.selection,photo,groups);organizer.anchor=photo;}
    };
    if(!organizer.toolbar && !organizer.targetBar && confirm && !cards.empty())choose(cards[organizer.focus].photo,false);
    if(!organizer.toolbar && !organizer.targetBar && pad(UI::ImGuiKey_GamepadR3) && !cards.empty()){organizer.context=cards[organizer.focus].photo;details=true;}
    for(int row=start;row<finish;++row)for(int col=0;col<organizerColumns;++col) {
        const int n=row*organizerColumns+col;if(n>=static_cast<int>(cards.size()))break;const auto& p=cards[n];
        UI::SetCursorPos({4.f+col*stepX,origin+row*stepY});UI::BeginGroup();
        const auto path=(std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/p.photo).generic_string();visible.push_back(path);
        auto texture=LoadPortraitView(path);const bool selected=organizer.selection.contains(p.photo);
        if(selected)UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
        // Match the sampled image aspect to the card without stretching.
        UI::ImVec2 uv0{0,0},uv1{1,1};
        if(texture) {
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            static_cast<ID3D11ShaderResourceView*>(texture)->GetResource(resource.GetAddressOf());
            Microsoft::WRL::ComPtr<ID3D11Texture2D> image;
            if(resource && SUCCEEDED(resource.As(&image))) {
                D3D11_TEXTURE2D_DESC desc{};image->GetDesc(&desc);
                if(desc.Width && desc.Height) {
                    const float aspect=static_cast<float>(desc.Width)/desc.Height,target=w/h;
                    if(aspect>target) {const float fraction=target/aspect;uv0.x=(1.f-fraction)*.5f;uv1.x=1.f-uv0.x;}
                    else {const float fraction=aspect/target;uv0.y=(1.f-fraction)*.5f;uv1.y=1.f-uv0.y;}
                }
            }
        }
        const bool clicked=texture?UI::ImageButton(("org"+p.photo).c_str(),texture,{w,h},uv0,uv1):UI::Button((std::string(Tr("Missing photo"))+"##org"+p.photo).c_str(),{w,h});
        if(selected)UI::PopStyleColor();
        const auto a=UI::GetItemRectMin(),b=UI::GetItemRectMax();auto* draw=UI::GetWindowDrawList();
        if(const auto g=groups.find(p.photo);g!=groups.end()) {
            const auto badge="+"+std::to_string(g->second.size()-1);
            const float bw=UI::CalcTextSize(badge.c_str()).x+12.f,bh=UI::GetTextLineHeightWithSpacing()+4.f;
            UI::ImDrawListManager::AddRectFilled(draw,{b.x-bw-5,b.y-bh-5},{b.x-5,b.y-5},0xEE202020,0,0);
            UI::ImDrawListManager::AddText(draw,{b.x-bw+1,b.y-bh-3},0xFFFFFFFF,badge.c_str());
            if(organizer.variation.photo==p.photo) {organizer.variationMin=a;organizer.variationMax=b;variationAnchor=true;}
            if(UI::IsItemHovered() && organizer.variation.photo.empty() && !UI::IsMouseDown(0) && !UI::GetDragDropPayload() && !organizer.destination) {
                variationHover=true;
                if(organizer.variation.clock.Ready(p.photo,now)) {organizer.variation.photo=p.photo;organizer.variationMin=a;organizer.variationMax=b;variationAnchor=true;openVariation=true;}
            }
        }
        if(selected) {UI::ImDrawListManager::AddRect(draw,a,b,0xFF70E8FF,0,0,3.f);UI::ImDrawListManager::AddRectFilled(draw,{a.x+5,a.y+5},{a.x+21,a.y+21},0xFF70E8FF,0,0);}
        if(n==organizer.focus && !organizer.toolbar && !organizer.targetBar)UI::ImDrawListManager::AddRect(draw,{a.x+3,a.y+3},{b.x-3,b.y-3},0xFFE0E0E0,0,0,1.f);
        if(missingCheckMode && PresetHasMissingData(missingResults,p.photo)) {UI::ImDrawListManager::AddLine(draw,{b.x-22,a.y+7},{b.x-7,a.y+22},0xFF6060FF,3.f);UI::ImDrawListManager::AddLine(draw,{b.x-7,a.y+7},{b.x-22,a.y+22},0xFF6060FF,3.f);}
        previewRects.push_back({n,a,b});
        if(organizer.destination && !selected && !conflict && !missingCheckMode &&
           (UI::IsItemHovered()))previewTarget=p.photo;
        if(UI::IsItemHovered() && !organizer.destination)UI::SetTooltip("%s",p.name.c_str());
        // A nonmodal popup blocks its parent widgets even where the source
        // remains exposed. Permit only this source card's blocked selection click.
        const bool sourceSelectionClick=organizer.variation.photo==p.photo && !organizer.variation.context &&
            !UI::IsItemHovered() && UI::IsItemHovered(UI::ImGuiHoveredFlags_AllowWhenBlockedByPopup) && UI::IsMouseClicked(0);
        // Popup-blocked parent clicks do not own ImGui's active widget ID.
        // Remember this exposed source gesture without changing ordinary click selection.
        if(sourceSelectionClick && !confirm && !conflict && !missingCheckMode &&
           !organizer.destination && !UI::GetDragDropPayload()) {
            organizer.representativeDragPhoto=p.photo;
            organizer.representativeDragSelection=Variations::DragSelection(organizer.selection,p.photo,groups);
        }
        if((clicked || sourceSelectionClick) && !confirm && !UI::GetDragDropPayload()){organizer.focus=n;organizer.toolbar=false;organizer.targetBar=false;choose(p.photo,UI::GetIO()->KeyShift);}
        if(UI::IsItemClicked(1)){
            if(!organizer.selection.contains(p.photo)) {organizer.selection.clear();Variations::ToggleSelection(organizer.selection,p.photo,groups);}
            organizer.context=p.photo;details=true;
        }
        if(!conflict && organizer.representativeDragPhoto.empty() && UI::IsMouseDragging(0,24.f) && UI::BeginDragDropSource()) {
            organizer.selection=Variations::DragSelection(organizer.selection,p.photo,groups);organizer.registrationTab.clear();
            const int token=1;UI::SetDragDropPayload("OUTFIT_ORGANIZE",&token,sizeof(token));UI::Text(Tr("Selected: %u / Visible: %u"),static_cast<unsigned>(organizer.selection.size()),static_cast<unsigned>(cards.size()));UI::EndDragDropSource();
        }
        if(!missingCheckMode && !conflict && !organizer.selection.contains(p.photo) && UI::BeginDragDropTarget()) {
            if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_ORGANIZE",UI::ImGuiDragDropFlags_AcceptBeforeDelivery|UI::ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                previewTarget=p.photo;
                if(payload->Delivery)moveTarget=p.photo;
            }UI::EndDragDropTarget();
        }
        const auto nextCursor=UI::GetCursorPos();
        UI::SetCursorPos({4.f+col*stepX-gap*.5f,origin+row*stepY});
        UI::InvisibleButton(("orggap"+p.photo).c_str(),{gap*.5f,h});
        if(!selected && !missingCheckMode && !conflict) {
            if(organizer.destination && UI::IsItemHovered())previewTarget=p.photo;
            if(organizer.destination && UI::IsItemClicked())moveTarget=p.photo;
            if(UI::BeginDragDropTarget()) {
                if(const auto* payload=UI::AcceptDragDropPayload("OUTFIT_ORGANIZE",UI::ImGuiDragDropFlags_AcceptBeforeDelivery|UI::ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                    previewTarget=p.photo;if(payload->Delivery)moveTarget=p.photo;
                }UI::EndDragDropTarget();
            }
        }
        UI::SetCursorPos(nextCursor);
        if(organizerShowNames) {auto name=p.name;while(!name.empty() && UI::CalcTextSize(name.c_str()).x>w) {auto last=name.size()-1;while(last && (static_cast<unsigned char>(name[last])&0xC0)==0x80)--last;name.resize(last);}UI::TextUnformatted(name.c_str());}UI::EndGroup();
    }
    // Shade the destination photos that will follow the inserted group.
    // These are identities, not final screen slots vacated by the source.
    if(!previewTarget.empty() && canMove) {
        auto* draw=UI::GetWindowDrawList();int number=0;
        std::map<std::string,int> placements;
        const auto target=std::find(view.begin(),view.end(),previewTarget);
        for(auto it=target;it!=view.end() && number<static_cast<int>(organizer.selection.size());++it)
            if(!organizer.selection.contains(*it))placements[*it]=++number;
        for(const auto& rect:previewRects) {
            if(cards[rect.index].photo==previewTarget)UI::ImDrawListManager::AddLine(draw,{rect.a.x-3,rect.a.y},{rect.a.x-3,rect.b.y},0xFF70E8FF,4.f);
            if(auto it=placements.find(cards[rect.index].photo);it!=placements.end()) {
                UI::ImDrawListManager::AddRectFilled(draw,rect.a,rect.b,0xAA606060,0,0);
                UI::ImDrawListManager::AddRect(draw,rect.a,rect.b,0xFF70E8FF,0,0,2.f);
                const auto label=std::to_string(it->second);
                UI::ImDrawListManager::AddText(draw,{rect.a.x+5,rect.a.y+5},0xFFFFFFFF,label.c_str());
            }
        }
    }
    UI::SetCursorPos({4,origin+rows*stepY});UI::Dummy({1,1});UI::EndChild();
    if(!organizer.representativeDragPhoto.empty()) {
        if(conflict || confirm || missingCheckMode || !groups.contains(organizer.representativeDragPhoto)) {
            organizer.representativeDragPhoto.clear();organizer.representativeDragSelection.clear();organizer.representativeDragging=false;
        } else {
            if(UI::IsMouseDown(0) && UI::IsMouseDragging(0,24.f)) {
                organizer.representativeDragging=true;
                organizer.selection=organizer.representativeDragSelection;
                organizer.registrationTab.clear();organizer.destination=false;
            }
            // Public external-source API avoids depending on private ImGui ActiveID fields.
            // Continue offscreen and through release so existing photo/tab drop targets deliver.
            if(organizer.representativeDragging && UI::BeginDragDropSource(UI::ImGuiDragDropFlags_SourceExtern)) {
                const int token=1;UI::SetDragDropPayload("OUTFIT_ORGANIZE",&token,sizeof(token));
                UI::Text(Tr("Selected: %u / Visible: %u"),static_cast<unsigned>(organizer.selection.size()),static_cast<unsigned>(cards.size()));
                UI::EndDragDropSource();
            }
            if(!UI::IsMouseDown(0)) {
                organizer.representativeDragPhoto.clear();organizer.representativeDragSelection.clear();organizer.representativeDragging=false;
            }
        }
    }
    if(organizer.variation.photo.empty() && !variationHover)organizer.variation.clock.candidate.clear();
    if(!organizer.variation.photo.empty() && (!variationAnchor || details || UI::GetDragDropPayload())) {
        if(UI::IsPopupOpen("Organizer variations")) {
            if(UI::BeginPopup("Organizer variations")){UI::CloseCurrentPopup();UI::EndPopup();}
        }
        organizer.variation={};
    }
    if(openVariation)UI::OpenPopup("Organizer variations");
    if(!organizer.variation.photo.empty()) {
        const auto& choices=groups.at(organizer.variation.photo);
        const auto* stripStyle=UI::GetStyle();
        auto strip=Variations::MakeStripLayout(w,h,choices.size(),dims.width*.9f,dims.height*.9f,
            stripStyle->WindowPadding.x,stripStyle->WindowPadding.y,stripStyle->FramePadding.x,stripStyle->FramePadding.y,
            stripStyle->ItemSpacing.x,stripStyle->ItemSpacing.y,UI::GetTextLineHeight(),stripStyle->ScrollbarSize,stripStyle->WindowBorderSize);
        const auto area=Variations::OrganizerStripArea(organizer.variationMin.y,strip.height,dims.height*.05f,dims.height*.95f,
            std::max(UiPx(32.f),UI::GetTextLineHeightWithSpacing()+UiPx(8.f)),UiPx(6.f));
        if(area.height<strip.height)
            strip=Variations::MakeStripLayout(w,h,choices.size(),dims.width*.9f,area.height,
                stripStyle->WindowPadding.x,stripStyle->WindowPadding.y,stripStyle->FramePadding.x,stripStyle->FramePadding.y,
                stripStyle->ItemSpacing.x,stripStyle->ItemSpacing.y,UI::GetTextLineHeight(),stripStyle->ScrollbarSize,stripStyle->WindowBorderSize);
        const float tw=strip.imageWidth,th=strip.imageHeight,width=strip.width,height=strip.height;
        const float x=std::clamp((organizer.variationMin.x+organizer.variationMax.x-width)*.5f,dims.width*.03f,dims.width*.97f-width);
        const float y=area.top;
        UI::SetNextWindowPos({x,y});UI::SetNextWindowSize({width,height});
        if(UI::BeginPopup("Organizer variations",UI::ImGuiWindowFlags_NoSavedSettings|UI::ImGuiWindowFlags_NoMove|UI::ImGuiWindowFlags_NoScrollbar|UI::ImGuiWindowFlags_NoScrollWithMouse|UI::ImGuiWindowFlags_NoNavInputs)) {
            // Paint only the existing heading row, preserving thumbnail geometry and input.
            const auto headingPos=UI::GetCursorScreenPos();
            const auto headingWindow=UI::GetWindowPos();
            UI::ImDrawListManager::AddRectFilled(UI::GetWindowDrawList(),headingPos,
                {headingWindow.x+UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x,
                 headingPos.y+UI::GetTextLineHeight()},0xFF4D3D2C,0,0);
            UI::TextUnformatted(Tr("Outfit variations"));
            UI::SetNextWindowContentSize({strip.contentWidth,strip.contentHeight});
            UI::PushStyleVar(UI::ImGuiStyleVar_WindowPadding,{0,0});
            UI::BeginChild(("Organizer variation photos##"+organizer.variation.photo).c_str(),{0,0},0,UI::ImGuiWindowFlags_HorizontalScrollbar|UI::ImGuiWindowFlags_NoScrollWithMouse|UI::ImGuiWindowFlags_NoNavInputs);
            if(!organizer.variation.context && UI::IsWindowHovered(UI::ImGuiHoveredFlags_RootAndChildWindows)) {
                const auto* wheelIO=UI::GetIO();
                if(wheelIO->MouseWheel!=0.f || wheelIO->MouseWheelH!=0.f)
                    UI::SetScrollX(Variations::WheelScroll(UI::GetScrollX(),UI::GetScrollMaxX(),strip.stride,wheelIO->MouseWheel,wheelIO->MouseWheelH));
            }
            const auto initial=UI::GetCursorPos();const float stride=strip.stride;
            const auto first=static_cast<std::size_t>(std::max(0.f,std::floor(UI::GetScrollX()/stride)-1));
            const auto last=std::min(choices.size(),first+static_cast<std::size_t>(width/stride)+3);
            for(std::size_t i=first;i<last;++i) {
                const auto& p=choices[i];UI::SetCursorPos({initial.x+i*stride,initial.y});UI::BeginGroup();
                const auto path=(std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Captures")/p.photo).generic_string();visible.push_back(path);
                const auto texture=LoadPortraitView(path);const auto id="orgvar"+p.photo;
                UI::ImVec2 uv0{0,0},uv1{1,1};
                if(texture) {
                    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
                    static_cast<ID3D11ShaderResourceView*>(texture)->GetResource(resource.GetAddressOf());
                    Microsoft::WRL::ComPtr<ID3D11Texture2D> image;
                    if(resource && SUCCEEDED(resource.As(&image))) {
                        D3D11_TEXTURE2D_DESC desc{};image->GetDesc(&desc);
                        if(desc.Width && desc.Height) {
                            const float aspect=static_cast<float>(desc.Width)/desc.Height,target=tw/th;
                            if(aspect>target) {const float fraction=target/aspect;uv0.x=(1-fraction)*.5f;uv1.x=1-uv0.x;}
                            else {const float fraction=aspect/target;uv0.y=(1-fraction)*.5f;uv1.y=1-uv0.y;}
                        }
                    }
                }
                const bool clicked=texture?UI::ImageButton(id.c_str(),texture,{tw,th},uv0,uv1):UI::Button(id.c_str(),{strip.cardWidth,strip.cardHeight});
                if(organizer.selection.contains(p.photo))UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF70E8FF,0,0,3.f);
                if(UI::IsItemHovered())UI::SetTooltip("%s",p.name.c_str());
                if(clicked){if(!organizer.selection.erase(p.photo))organizer.selection.insert(p.photo);}
                if(UI::IsItemClicked(1)){organizer.context=p.photo;variationDetails=true;organizer.variation.context=true;}
                UI::Text("%u / %u",static_cast<unsigned>(i+1),static_cast<unsigned>(choices.size()));UI::EndGroup();
            }
            UI::EndChild();UI::PopStyleVar();
            if(variationDetails)UI::OpenPopup("Organizer variation actions");
            const bool variationContext=UI::IsPopupOpen("Organizer variation actions");
            if(organizer.variation.context && !variationContext) {organizer.variation.context=false;organizer.variation.clock.outside=0;}
            renderDetails("Organizer variation actions");
            const auto mouse=UI::GetIO()->MousePos;
            const auto in=[&](UI::ImVec2 a,UI::ImVec2 b){return mouse.x>=a.x-6&&mouse.x<=b.x+6&&mouse.y>=a.y-6&&mouse.y<=b.y+6;};
            if(details || (!variationContext && organizer.variation.clock.Leave(in({x,y},{x+width,y+height})||in(organizer.variationMin,organizer.variationMax),now))) {UI::CloseCurrentPopup();organizer.variation={};}
            UI::EndPopup();
        } else if(!UI::IsPopupOpen("Organizer variations"))organizer.variation={};
    }
    if(!moveTarget.empty() && canMove) {moveSelection(moveTarget);organizer.destination=false;}
    for(const auto& path:organizer.loaded)if(std::find(visible.begin(),visible.end(),path)==visible.end())DisposePortraitView(path);organizer.loaded=std::move(visible);
    if(details)UI::OpenPopup("Organizer details");
    renderDetails("Organizer details");
    if(!categoryRemovals.empty() && canTrash) {
        const auto removed=Organize::RemoveFromCategory(organizer.draft,categoryRemovals,organizer.category);
        if(removed) {
            organizer.registrationTab=CategoryTitle(tabs[tab]);
            organizer.registrationRemoved=static_cast<unsigned>(removed);
            for(const auto& photo:categoryRemovals)organizer.selection.erase(photo);
            organizer.anchor.clear();organizer.destination=false;
        }
    }
    if(!trashTargets.empty() && canTrash) {
        for(const auto& photo:trashTargets) {
            organizer.draft.labels[photo].deleted=organizer.category!="#trash";
            organizer.selection.erase(photo);
        }
        organizer.anchor.clear();organizer.destination=false;organizer.registrationTab.clear();
    }
    if(button("Save and close",8,!conflict && libraryWritable))save=true;UI::SameLine();
    if(button("Cancel",9))close=true;
    UI::SameLine(0,UI::GetStyle()->ItemSpacing.x*3.f);
UI::TextWrapped("%s",Tr("Select photos without equipping. Changes are applied only when saved. Filtered views keep hidden photos in place."));
    if(save) {
        try {
            if(OrganizerDisk()!=organizer.disk)organizer.error=Tr("Library.json changed on disk. Cancel and reopen before saving.");
            else if(CommitLibrary(organizer.draft))close=true;
            else organizer.error=Tr("Could not save. Your draft is still open.");
        }catch(const std::exception& e){organizer.error=e.what();}
    }
    if(close){UI::CloseCurrentPopup();EndOrganizer();}
    UI::EndPopup();
}
