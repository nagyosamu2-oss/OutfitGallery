void BeginEquipmentSection(const char* id,const char* title,bool gold) {
    UI::PushStyleColor(UI::ImGuiCol_ChildBg,gold?UI::ImVec4{.35f,.27f,.10f,.16f}:UI::ImVec4{.12f,.26f,.36f,.16f});
    UI::PushStyleColor(UI::ImGuiCol_Border,gold?UI::ImVec4{.72f,.59f,.32f,.7f}:UI::ImVec4{.40f,.65f,.80f,.7f});
    UI::PushStyleVar(UI::ImGuiStyleVar_ChildBorderSize,2.f);
    UI::BeginChild(id,{0,0},UI::ImGuiChildFlags_Border|UI::ImGuiChildFlags_AutoResizeY|UI::ImGuiChildFlags_AlwaysUseWindowPadding);
    UI::PopStyleVar();UI::PopStyleColor(2);
    UI::TextUnformatted(Tr(title));UI::Separator();
}
void PrepareEquipmentDialog() {
    const auto area=uiLayoutScreen;
    UI::SetNextWindowSize({std::min(UiPx(820.f),area.x*.72f),area.y*.78f},UI::ImGuiCond_Appearing);
    UI::SetNextWindowPos({area.x*.5f,area.y*.5f},UI::ImGuiCond_Appearing,{.5f,.5f});
    UI::SetNextWindowSizeConstraints({std::min(UiPx(480.f),area.x*.90f),std::min(UiPx(300.f),area.y*.90f)},{area.x*.95f,area.y*.95f});
}
bool RenderPresetEditor(const Preset& target,bool allowed) {
    bool closeParent=false;
    static std::vector<unsigned> selection;
    static bool confirming=false;
    static bool restoring=false;
    UI::BeginDisabled(!allowed || target.slotMask || target.accessories || saveBusy || editPending);
    if(UI::Button(Label("Add or replace from worn equipment"))) {
        selection.clear();confirming=false;restoring=false;
        {std::scoped_lock lock(editMutex);editSnapshot={};editPhoto=target.photo;editSelection.clear();editSession=studioOpenSerial.load();}
        editPending=true;command=11;UI::OpenPopup(Label("Preset equipment editor"));
    }
    if(UI::Button(Label("Restore first-edit equipment"))) {
        selection.clear();confirming=false;restoring=true;
        {std::scoped_lock lock(editMutex);editSnapshot={};editPhoto=target.photo;editSelection.clear();editSession=studioOpenSerial.load();}
        editPending=true;command=13;UI::OpenPopup(Label("Preset equipment editor"));
    }
    UI::EndDisabled();
    if(target.slotMask || target.accessories) UI::TextWrapped("%s",Tr("Editing requires a full outfit preset."));
    PrepareEquipmentDialog();
    bool editorOpen=true;
    const bool editorWasOpen=UI::IsPopupOpen(Label("Preset equipment editor"));
    if(UI::BeginPopupModal(Label("Preset equipment editor"),editPending?nullptr:&editorOpen,UI::ImGuiWindowFlags_NoSavedSettings)) {
        EditSnapshot state;{std::scoped_lock lock(editMutex);state=editSnapshot;}
        UI::BeginChild("Editor body",{0,-UI::GetFrameHeightWithSpacing()-UI::GetStyle()->ItemSpacing.y});
        UI::TextUnformatted(target.name.c_str());
        if(!state.ready || editPending) UI::TextUnformatted(Tr("Loading equipment slots..."));
        else if(state.saved) {
            UI::TextWrapped("%s",Tr("Preset updated. Photo and assignments were kept."));
            UI::TextWrapped("%s",Tr("The first-edit equipment record is kept. No edit history is created. Photo and inventory are unchanged."));
            if(state.registeredEnchanted) UI::TextWrapped("%s",Tr("Custom enchanted items were registered. Save your game to retain their identities."));
        } else if(!state.error.empty()) UI::TextWrapped("%s",Tr(state.error.c_str()));
        else if(restoring && state.origin) {
            UI::TextWrapped("%s",Tr(state.origin->legacy?"This is the oldest available edit backup; it may not match the original photo.":"Restore the equipment recorded before the first edit. The photo is kept."));
            UI::TextUnformatted(Tr("Equipment to restore:"));
            for(const auto& item:state.origin->preset.items) UI::TextWrapped("- %s",item.name.c_str());
            UI::TextWrapped("%s",Tr("This changes the saved preset only. Current equipment and exchange-slot limits are unchanged."));
            if(UI::Button(Label("Confirm equipment restoration"))) {editPending=true;command=14;}
        }
        else {
            UI::TextUnformatted(Tr("Select equipment to add or replace"));
            const float choicesHeight=std::max(UI::GetFrameHeightWithSpacing()*2.f,UI::GetContentRegionAvail().y*(confirming?.44f:.68f));
            UI::BeginChild("Worn editor choices",{0,choicesHeight});
            for(unsigned n=0;n<state.worn.items.size();++n) {
                const auto& item=state.worn.items[n];bool selected=std::find(selection.begin(),selection.end(),n)!=selection.end();
                UI::PushID(static_cast<int>(n));UI::BeginDisabled(confirming);
                if(UI::Checkbox("##selectWorn",&selected)) {if(selected)selection.push_back(n);else std::erase(selection,n);}
                UI::EndDisabled();UI::SameLine();
                std::string slots;for(unsigned bit=0;bit<32;++bit)if(state.wornMasks[n]&(1u<<bit)) {if(!slots.empty())slots+=", ";slots+=std::to_string(bit+30);}
                UI::TextWrapped("[%s] %s%s",slots.empty()?Tr("No biped slot"):slots.c_str(),item.name.c_str(),item.preferred.custom?Tr(" / Custom enchanted"):"");
                UI::PopID();
            }
            UI::EndChild();
            std::optional<PresetEditMerge> merged;std::string error;
            if(!selection.empty())try{merged=MergePresetEquipment(state.file.original,state.oldMasks,state.worn,state.wornMasks,selection);}catch(const std::exception& e){error=e.what();}
            if(!error.empty())UI::TextWrapped("%s",Tr(error.c_str()));
            if(confirming && merged) {
                UI::TextUnformatted(Tr("Changes to the saved preset"));
                UI::BeginChild("Edit changes",{0,std::max(UI::GetFrameHeightWithSpacing()*2.f,UI::GetContentRegionAvail().y*.55f)});
                UI::TextUnformatted(Tr("Add or update:"));for(auto n:merged->added)UI::TextWrapped("- %s",state.worn.items[n].name.c_str());
                UI::TextWrapped("%s",Tr("Remove from preset (inventory is unchanged):"));
                if(merged->removed.empty())UI::TextUnformatted(Tr("None"));
                for(auto n:merged->removed)UI::TextWrapped("- %s",state.file.original.items[n].name.c_str());
                UI::EndChild();
                if(UI::Button(Label("Save edited preset"))) {
                    {std::scoped_lock lock(editMutex);editSelection=selection;}
                    editPending=true;command=12;
                }
                UI::SameLine();if(UI::Button(Label("Back")))confirming=false;
            } else {
                UI::BeginDisabled(!merged.has_value());if(UI::Button(Label("Review changes")))confirming=true;UI::EndDisabled();
            }
            UI::Spacing();UI::Separator();
            if(UI::CollapsingHeader(Label("Equipment help"))) {
                UI::TextWrapped("%s",Tr("Select worn armor or clothing to add. Conflicting registered items are replaced as whole items."));
                UI::TextWrapped("%s",Tr("Slotless equipment can be added to full outfits. It is excluded when exchange slots are limited."));
                UI::TextWrapped("%s",Tr("Photo stays unchanged. Existing exchange-slot limits still apply. Equipment changes on the next outfit application."));
            }
        }
        UI::EndChild();
        UI::BeginDisabled(editPending);
        if(UI::Button(Label(state.saved?"Close":"Cancel"))) {closeParent=state.saved;UI::CloseCurrentPopup();}
        UI::EndDisabled();
        UI::EndPopup();
    }
    // Some ImGui versions also clear p_open for an already-closed modal.
    // Only a real open-to-closed transition may close its parent.
    if(editorWasOpen && !editorOpen) {std::scoped_lock lock(editMutex);closeParent=editSnapshot.saved;}
    if(closeParent) {std::scoped_lock lock(editMutex);editSnapshot.saved=false;}
    return closeParent;
}
