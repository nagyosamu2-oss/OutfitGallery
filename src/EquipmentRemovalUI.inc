void RenderEquipmentRemoval(bool compact=false) {
    {std::scoped_lock lock(removalMutex);
     if(removalState.session!=studioOpenSerial.load()) {removalPending=false;removalState.waiting=false;}}
    UI::BeginDisabled(saveBusy || verificationTicks || removalPending || photoRetakeActive);
    const bool pressed=UI::Button(compact?"-##removeEquipment":Label("Remove equipment"),compact?UI::ImVec2{UI::GetFrameHeight(),0}:UI::ImVec2{0,0});
    if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Remove equipment"));
    if(pressed) {
        {std::scoped_lock lock(removalMutex);removalState={};
         removalState.session=studioOpenSerial.load();removalState.actor=studioActorID.load();
         removalState.appearance=TargetAppearanceMode(removalState.actor);}
        removalPending=true;command=34;UI::OpenPopup(Label("Current equipment removal"));
    }
    UI::EndDisabled();
    PrepareEquipmentDialog();bool open=true;
    if(UI::BeginPopupModal(Label("Current equipment removal"),removalPending?nullptr:&open,UI::ImGuiWindowFlags_NoSavedSettings)) {
        EquipmentRemovalState state;{std::scoped_lock lock(removalMutex);state=removalState;}
        UI::TextUnformatted(Tr(state.appearance?"Appearance equipment":"Currently worn equipment"));
        UI::TextWrapped("%s",Tr("Check to remove immediately; uncheck to restore that item. Closing keeps the current state. Removing clothing may leave the character in underwear or naked."));
        if(state.appearance) UI::TextWrapped("%s",Tr("Only applied appearance items are listed. Actual equipment and its effects are kept."));
        if(!state.error.empty()) UI::TextWrapped("%s",Tr(state.error.c_str()));
        UI::BeginDisabled(removalPending || verificationTicks);
        if(UI::Button(Label("Refresh equipment list"))) {
            {std::scoped_lock lock(removalMutex);removalState.error.clear();removalState.ready=false;
             removalState.appearance=TargetAppearanceMode(removalState.actor);}
            removalPending=true;command=34;
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Refreshing starts a new list. Items already removed will no longer be listed."));
        UI::EndDisabled();
        UI::BeginChild("Removal choices",{0,-UI::GetFrameHeightWithSpacing()-UI::GetStyle()->ItemSpacing.y});
        if(!state.ready) UI::TextUnformatted(Tr("Loading equipment slots..."));
        else if(state.rows.empty()) UI::TextUnformatted(Tr("No removable equipment."));
        else {
            UI::BeginDisabled(removalPending || !state.error.empty() || verificationTicks);
            for(unsigned n=0;n<state.rows.size();++n) {
                const auto& row=state.rows[n];bool removed=state.removed.contains(n);
                std::string slots;
                for(unsigned bit=0;bit<32;++bit) if(row.slots&(1u<<bit)) {
                    if(!slots.empty()) slots+=", ";slots+=std::to_string(bit+30);
                }
                UI::PushID(static_cast<int>(n));
                if(UI::Checkbox("##removeChoice",&removed)) {
                    {std::scoped_lock lock(removalMutex);removalState.actionRow=n;removalState.removeWanted=removed;}
                    removalPending=true;command=35;
                }
                UI::SameLine();UI::TextWrapped("[%s] %s",slots.empty()?Tr("No biped slot"):slots.c_str(),row.name.c_str());
                UI::PopID();
            }
            UI::EndDisabled();
        }
        UI::EndChild();
        UI::BeginDisabled(removalPending);
        if(UI::Button(Label("Close")) || !open) UI::CloseCurrentPopup();
        UI::EndDisabled();UI::EndPopup();
    }
}
