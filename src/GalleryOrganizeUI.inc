// Render-thread-owned transaction. Equipment and preset files are never changed here.
struct OrganizerState {
    bool active{},openPopup{},destination{},toolbar{};
    int action{},focus{};
    unsigned revision{},presetsRevision{},opening{};
    Library draft;
    std::vector<Preset> snapshot;
    std::set<std::string> selection;
    std::string category,anchor,context,error,filter;
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
    std::vector<Category> tabs{{"","All"}};
    tabs.insert(tabs.end(),library.categories.begin(),library.categories.end());
    tabs.push_back({"#head","Head equipment"});tabs.push_back({"#accessories","Accessories"});tabs.push_back({"#legacy","Legacy parts"});tabs.push_back({"#trash","Trash"});
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
        return deleted!=(organizer.category=="#trash") || !CollectionFits(p.slotMask,organizer.category,p.accessories) ||
            (!organizer.category.empty() && !organizer.category.starts_with("#") && (it==organizer.draft.labels.end() || std::find(it->second.categories.begin(),it->second.categories.end(),organizer.category)==it->second.categories.end())) ||
            (organizer.search[0] && p.name.find(organizer.search)==std::string::npos);
    });
    const auto filter=organizer.category+"|"+organizer.search;
    const bool reset=filter!=organizer.filter;
    if(reset) {organizer.filter=filter;organizer.selection.clear();organizer.anchor.clear();organizer.destination=false;organizer.focus=0;}
    if(missingCheckMode && !missingCheckPending && missingCheckError.empty())MissingPresetsFirst(cards,missingResults);
    std::vector<std::string> view;for(const auto& p:cards)view.push_back(p.photo);
    const bool padOK=!UI::GetIO()->WantTextInput && !UI::IsAnyItemActive() && !UI::IsPopupOpen("Organizer details");
    auto pad=[&](auto key,bool repeat=false){return padOK && PadPressed(key,repeat);};
    const bool confirm=pad(UI::ImGuiKey_GamepadFaceDown);
    const bool left=pad(UI::ImGuiKey_GamepadDpadLeft,true)||pad(UI::ImGuiKey_GamepadLStickLeft,true);
    const bool right=pad(UI::ImGuiKey_GamepadDpadRight,true)||pad(UI::ImGuiKey_GamepadLStickRight,true);
    const bool up=pad(UI::ImGuiKey_GamepadDpadUp,true)||pad(UI::ImGuiKey_GamepadLStickUp,true);
    const bool down=pad(UI::ImGuiKey_GamepadDpadDown,true)||pad(UI::ImGuiKey_GamepadLStickDown,true);
    bool scrolled=false;
    if(organizer.toolbar) {
        if(left||right)organizer.action=(organizer.action+(right?1:9))%10;
        if(down){organizer.toolbar=false;scrolled=true;}
    } else {
        if(up && organizer.focus<organizerColumns)organizer.toolbar=true;
        else if(up){organizer.focus-=organizerColumns;scrolled=true;}
        if(down){organizer.focus+=organizerColumns;scrolled=true;}
        if(left){--organizer.focus;scrolled=true;}if(right){++organizer.focus;scrolled=true;}
    }
    organizer.focus=std::clamp(organizer.focus,0,std::max(0,static_cast<int>(cards.size())-1));
    if(pad(UI::ImGuiKey_GamepadL1)||pad(UI::ImGuiKey_GamepadR1)) organizer.category=tabs[(tab+(pad(UI::ImGuiKey_GamepadR1)?1:static_cast<int>(tabs.size())-1))%tabs.size()].id;
    if(!UI::IsPopupOpen("Organizer details") && (pad(UI::ImGuiKey_GamepadFaceRight)||UI::IsKeyPressed(UI::ImGuiKey_Escape,false))) {
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
            organizer.selection.clear();organizer.anchor.clear();organizer.destination=false;
        }
    };
    std::vector<std::string> trashTargets;
    const bool canTrash=!conflict && libraryWritable;
    auto trashLabel=[&](std::size_t count) {
        return std::string(Tr(organizer.category=="#trash"?"Restore from Trash":"Move to Trash"))+" ("+std::to_string(count)+")";
    };
    const bool canMove=!missingCheckMode && !organizer.selection.empty() && !conflict;
    if(button("Select visible",0))organizer.selection.insert(view.begin(),view.end());UI::SameLine();
    if(button("Clear selection",1)){organizer.selection.clear();organizer.destination=false;}UI::SameLine();
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
    if(missingCheckMode) {
        UI::TextWrapped("%s",Tr("Turn off missing gear checking to reorder. Your draft and selection are kept."));
        if(missingCheckPending)UI::TextUnformatted(Tr("Missing-data check mode: checking..."));
        else if(!missingCheckError.empty())UI::TextWrapped("%s",missingCheckError.c_str());
        else {auto counts=CountMissingPresets(organizer.snapshot,missingResults,[](const std::string& p){auto it=organizer.draft.labels.find(p);return it!=organizer.draft.labels.end()&&it->second.deleted;});UI::Text(Tr("Missing-data check mode: %u sets / %u in Trash"),counts.active,counts.trash);}
    } else if(organizer.destination) UI::TextWrapped("%s",Tr("Choose a photo or its left gap. Selected photos are inserted before the shaded photos, which follow them to the right. Escape/B cancels."));
    else UI::TextWrapped("%s",Tr("Click to select; Shift-click selects a range. Drag selected photos together. Pad: arrows/A; up from first row for actions; LB/RB for categories."));
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
    if(organizer.destination && !organizer.toolbar && !cards.empty() && !organizer.selection.contains(cards[organizer.focus].photo))previewTarget=cards[organizer.focus].photo;
    bool details=false;
    auto choose=[&](const std::string& photo,bool range) {
        if(organizer.destination && !missingCheckMode) {moveTarget=photo;return;}
        if(range)Organize::SelectRange(organizer.selection,view,organizer.anchor,photo);
        else {if(!organizer.selection.erase(photo))organizer.selection.insert(photo);organizer.anchor=photo;}
    };
    if(!organizer.toolbar && confirm && !cards.empty())choose(cards[organizer.focus].photo,false);
    if(!organizer.toolbar && pad(UI::ImGuiKey_GamepadR3) && !cards.empty()){organizer.context=cards[organizer.focus].photo;details=true;}
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
        if(selected) {UI::ImDrawListManager::AddRect(draw,a,b,0xFF70E8FF,0,0,3.f);UI::ImDrawListManager::AddRectFilled(draw,{a.x+5,a.y+5},{a.x+21,a.y+21},0xFF70E8FF,0,0);}
        if(n==organizer.focus && !organizer.toolbar)UI::ImDrawListManager::AddRect(draw,{a.x+3,a.y+3},{b.x-3,b.y-3},0xFFE0E0E0,0,0,1.f);
        if(missingCheckMode && PresetHasMissingData(missingResults,p.photo)) {UI::ImDrawListManager::AddLine(draw,{b.x-22,a.y+7},{b.x-7,a.y+22},0xFF6060FF,3.f);UI::ImDrawListManager::AddLine(draw,{b.x-7,a.y+7},{b.x-22,a.y+22},0xFF6060FF,3.f);}
        previewRects.push_back({n,a,b});
        if(organizer.destination && !selected && !conflict && !missingCheckMode &&
           (UI::IsItemHovered()))previewTarget=p.photo;
        if(UI::IsItemHovered() && !organizer.destination)UI::SetTooltip("%s",p.name.c_str());
        if(clicked && !confirm && !UI::GetDragDropPayload()){organizer.focus=n;organizer.toolbar=false;choose(p.photo,UI::GetIO()->KeyShift);}
        if(UI::IsItemClicked(1)){organizer.context=p.photo;details=true;}
        if(!missingCheckMode && !conflict && UI::IsMouseDragging(0,24.f) && UI::BeginDragDropSource()) {
            if(!selected){organizer.selection.clear();organizer.selection.insert(p.photo);}
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
        std::map<int,int> placements;
        const auto target=std::find(view.begin(),view.end(),previewTarget);
        for(auto it=target;it!=view.end() && number<static_cast<int>(organizer.selection.size());++it)
            if(!organizer.selection.contains(*it))placements[static_cast<int>(it-view.begin())]=++number;
        for(const auto& rect:previewRects) {
            if(cards[rect.index].photo==previewTarget)UI::ImDrawListManager::AddLine(draw,{rect.a.x-3,rect.a.y},{rect.a.x-3,rect.b.y},0xFF70E8FF,4.f);
            if(auto it=placements.find(rect.index);it!=placements.end()) {
                UI::ImDrawListManager::AddRectFilled(draw,rect.a,rect.b,0xAA606060,0,0);
                UI::ImDrawListManager::AddRect(draw,rect.a,rect.b,0xFF70E8FF,0,0,2.f);
                const auto label=std::to_string(it->second);
                UI::ImDrawListManager::AddText(draw,{rect.a.x+5,rect.a.y+5},0xFFFFFFFF,label.c_str());
            }
        }
    }
    UI::SetCursorPos({4,origin+rows*stepY});UI::Dummy({1,1});UI::EndChild();
    if(!moveTarget.empty() && canMove) {moveSelection(moveTarget);organizer.destination=false;}
    for(const auto& path:organizer.loaded)if(std::find(visible.begin(),visible.end(),path)==visible.end())DisposePortraitView(path);organizer.loaded=std::move(visible);
    if(details)UI::OpenPopup("Organizer details");
    if(UI::BeginPopup("Organizer details")) {
        const auto found=std::find_if(cards.begin(),cards.end(),[](const auto& p){return p.photo==organizer.context;});
        if(found!=cards.end())UI::TextWrapped("%s",found->name.c_str());
        if(missingCheckMode && !missingCheckPending) {
            auto it=missingResults.find(organizer.context);
            if(it!=missingResults.end())for(const auto& item:it->second){UI::TextWrapped("%s",item.name.c_str());UI::TextWrapped("%s / %06X",item.plugin.c_str(),item.localID);}
        }
        const auto targets=Organize::TrashTargets(view,organizer.selection,organizer.context);
        UI::BeginDisabled(!canTrash || targets.empty());
        if(UI::Button((trashLabel(targets.size())+"##ContextTrash").c_str())) {
            trashTargets=targets;UI::CloseCurrentPopup();
        }
        UI::EndDisabled();
        UI::TextWrapped("%s",Tr("Trash changes are also applied only when saved."));UI::EndPopup();
    }
    if(!trashTargets.empty() && canTrash) {
        for(const auto& photo:trashTargets) {
            organizer.draft.labels[photo].deleted=organizer.category!="#trash";
            organizer.selection.erase(photo);
        }
        organizer.anchor.clear();organizer.destination=false;
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
