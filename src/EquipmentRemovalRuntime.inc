struct EquipmentRemovalState {
    std::vector<RemovalItem> rows;
    std::set<unsigned> removed;
    std::vector<unsigned> queuedRows;
    std::string error;
    unsigned session{},actionRow{};
    std::uint32_t actor{};
    std::uint64_t deadline{};
    bool appearance{},ready{},removeWanted{},waiting{},maintained{};
};
std::mutex removalMutex;
EquipmentRemovalState removalState;
std::atomic<bool> removalPending{};
void ProcessEquipmentRemoval(unsigned request,RE::Actor* actor) {
    if(request!=34 && request!=35 && !removalPending.load()) return;
    EquipmentRemovalState state;
    {std::scoped_lock lock(removalMutex);state=removalState;}
    if(request!=34 && request!=35 && !state.waiting) return;
    try {
        if(!actor || state.session!=studioOpenSerial.load() || state.actor!=actor->GetFormID() ||
            state.appearance!=TargetAppearanceMode(actor->GetFormID()) || saveBusy || verificationTicks)
            throw std::runtime_error("Equipment changed. Refresh the list.");
        const auto current=state.appearance?SnapshotRemovalAppearance(actor):SnapshotRemovalEquipment(actor);
        bool applyNext=false;
        const bool wasWaiting=state.waiting;
        if(state.waiting) {
            const auto id=state.rows.at(state.actionRow).id;
            const bool present=std::any_of(current.begin(),current.end(),[&](const auto& r){return r.id==id;});
            if(present==!state.removeWanted) {
                if(state.removeWanted) state.removed.insert(state.actionRow);else state.removed.erase(state.actionRow);
                state.waiting=false;
                if(!state.appearance) {
                    if(state.maintained && !current.empty()) RememberFollowerOutfit(actor);
                    else RefreshFollowerAfterAccessoryRemoval(actor);
                }
                if(!state.queuedRows.empty()) {
                    state.actionRow=state.queuedRows.front();state.queuedRows.erase(state.queuedRows.begin());applyNext=true;
                }
                SetStatus(state.removeWanted?"Equipment set removed.":"Equipment restored.");
            } else if(GetTickCount64()<state.deadline) return;
            else throw std::runtime_error("Equipment change could not be confirmed. Refresh the list.");
        }
        if(!state.waiting && request==34) {
            state.rows=current;state.removed.clear();state.queuedRows.clear();state.error.clear();state.ready=true;
            state.maintained=IsFollowerOutfitMaintained(actor);
            if(state.appearance) BeginRemovalAppearance(actor);else BeginRemovalEquipment(actor);
        } else if(!state.waiting && ((request==35 && !wasWaiting) || applyNext)) {
            if(!state.ready || !state.error.empty() || state.actionRow>=state.rows.size())
                throw std::runtime_error("Equipment changed. Refresh the list.");
            // Other equipment must still match this session. No implicit replacement on restore.
            if(!RemovalCurrentMatches(state.rows,state.removed,current))
                throw std::runtime_error("Equipment changed. Refresh the list.");
            const auto id=state.rows[state.actionRow].id;
            const bool ok=state.removeWanted ?
                (state.appearance?RemoveAppearanceIDs(actor,{id}):RemoveEquipmentIDs(actor,{id})) :
                (state.appearance?RestoreRemovalAppearance(actor,id):RestoreRemovalEquipment(actor,id));
            if(!ok) throw std::runtime_error("Equipment could not be restored or removed. Refresh the list.");
            if(state.appearance) {
                SetTransmogAutoOwned(actor,false);if(actor->GetFormID()==0x14) autoAppearanceOwned=false;
            }
            AutoManualChange(actor);
            state.waiting=true;state.deadline=GetTickCount64()+750;equipmentLaneDue=state.deadline;
        }
    } catch(const std::exception& e) {state.ready=true;state.waiting=false;state.queuedRows.clear();state.error=e.what();}
    const bool pending=state.waiting;
    {std::scoped_lock lock(removalMutex);removalState=std::move(state);}
    removalPending=pending;
}
