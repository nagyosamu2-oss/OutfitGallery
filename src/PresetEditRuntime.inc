// Value-only request/response mailbox. Engine reads and identity registration stay on the game thread.
struct EditSnapshot {
    PresetEditFile file;
    Preset worn;
    std::vector<std::uint32_t> oldMasks,wornMasks;
    std::string error;
    bool ready{},saved{},registeredEnchanted{};
    std::optional<PresetEditOrigin> origin;
};
std::mutex editMutex;
EditSnapshot editSnapshot;
std::string editPhoto;
std::vector<unsigned> editSelection;
unsigned editSession{};

void ProcessPresetEdit(unsigned request,RE::Actor* actor) {
    if(request<11 || request>14) return;
    EditSnapshot state;std::vector<unsigned> selection;std::string photo;unsigned session;
    {std::scoped_lock lock(editMutex);state=editSnapshot;selection=editSelection;photo=editPhoto;session=editSession;}
    try {
        if(session!=studioOpenSerial.load()) throw std::runtime_error("Editor session expired. Reopen the editor.");
        if(request==11 || request==13) {
            state={};state.file=FindPresetForEditing(presetFolder,photo);
            if(state.file.original.slotMask || state.file.original.accessories) throw std::runtime_error("Editing requires a full outfit preset.");
            if(request==13) {
                state.origin=ReadPresetEditOrigin(state.file.path,photo);
                if(!state.origin)throw std::runtime_error("No original equipment record is available.");
                (void)ResolvePresetSlots(state.origin->preset);
            } else {
            state.oldMasks=ResolvePresetSlots(state.file.original);
            state.worn=SnapshotEquipment(actor,0,false,true);
            RememberSelectedEnchanted(actor,state.worn,false);
            state.wornMasks=ResolvePresetSlots(state.worn);
            }
        } else if(request==14) {
            if(!state.ready || !state.error.empty() || !state.origin)throw std::runtime_error("Reopen the editor before saving.");
            auto restored=state.file.original;restored.items=state.origin->preset.items;
            (void)ResolvePresetSlots(restored);
            const auto saved=SaveEditedPreset(state.file,restored);restored.equipmentEdited=saved.equipmentEdited;
            {std::scoped_lock lock(presetsMutex);for(auto& p:presets)if(p.photo==photo)p=restored;++presetRevision;}
            state.saved=true;
            SKSE::log::info("OG-EDIT restored photo={} origin={}",photo,saved.origin.generic_string());
            SetStatus("Original equipment restored. Photo and assignments were kept.");
        } else {
            if(!state.ready || !state.error.empty()) throw std::runtime_error("Reopen the editor before saving.");
            auto current=SnapshotEquipment(actor,0,false,true);RememberSelectedEnchanted(actor,current,false);
            const auto currentMasks=ResolvePresetSlots(current);
            Preset chosen;std::vector<std::uint32_t> chosenMasks;
            for(auto index:selection) {
                if(index>=state.worn.items.size()) throw std::runtime_error("Invalid equipment snapshot.");
                const auto& expected=state.worn.items[index];
                auto it=std::find_if(current.items.begin(),current.items.end(),[&](const auto& item){return SameBaseItem(expected,item);});
                if(it==current.items.end() || currentMasks[it-current.items.begin()]!=state.wornMasks[index] ||
                   it->registeredName!=expected.registeredName || it->preferred.custom!=expected.preferred.custom || it->preferred.signature!=expected.preferred.signature)
                    throw std::runtime_error("Worn equipment changed. Reopen the editor.");
                chosen.items.push_back(*it);chosenMasks.push_back(currentMasks[it-current.items.begin()]);
            }
            auto oldMasks=ResolvePresetSlots(state.file.original);
            if(oldMasks!=state.oldMasks) throw std::runtime_error("Equipment slots changed. Reopen the editor.");
            std::vector<unsigned> all;for(unsigned n=0;n<chosen.items.size();++n)all.push_back(n);
            (void)MergePresetEquipment(state.file.original,oldMasks,chosen,chosenMasks,all);
            // Confirm the backing file still represents this draft before adding any identity tags.
            const auto latest=FindPresetForEditing(presetFolder,photo);
            if(latest.path!=state.file.path || latest.bytes!=state.file.bytes) throw std::runtime_error("Preset changed. Reopen the editor.");
            RememberSelectedEnchanted(actor,chosen,true);
            for(const auto& item:chosen.items) if(item.preferred.custom && (!item.preferred.id || !item.preferred.scope))
                throw std::runtime_error("Enchanted item could not be registered safely. Preset was not changed.");
            auto merged=MergePresetEquipment(state.file.original,oldMasks,chosen,chosenMasks,all);
            const auto saved=SaveEditedPreset(state.file,merged.result);
            merged.result.equipmentEdited=saved.equipmentEdited;
            state.registeredEnchanted=std::any_of(chosen.items.begin(),chosen.items.end(),[](const auto& i){return i.preferred.custom;});
            {std::scoped_lock lock(presetsMutex);for(auto& p:presets)if(p.photo==photo)p=merged.result;++presetRevision;}
            state.saved=true;
            SKSE::log::info("OG-EDIT saved photo={} selected={} replaced={} origin={}",photo,selection.size(),merged.removed.size(),saved.origin.generic_string());
            SetStatus("Preset updated. Photo and assignments were kept.");
        }
    }catch(const std::exception& e){state.error=e.what();SKSE::log::warn("OG-EDIT {}",state.error);}
    state.ready=true;
    {std::scoped_lock lock(editMutex);if(editSession==session && editPhoto==photo)editSnapshot=std::move(state);}
    editPending=false;
}
