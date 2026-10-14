// Diagnostic-only, read-only. Snapshot runs in the existing game-thread poll;
// event sink only copies event IDs and never enumerates inventory or changes it.
struct FollowerArrivalEvents final : RE::BSTEventSink<RE::TESEquipEvent>, RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    std::mutex mutex;
    ULONGLONG second{};
    unsigned count{}, dropped{};
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* e,RE::BSTEventSource<RE::TESEquipEvent>*) override {
        if(!DetailEnabled() || !e || !e->actor || !FollowerAutomaticOwns(e->actor->GetFormID())) return RE::BSEventNotifyControl::kContinue;
        const auto now=GetTickCount64();
        std::scoped_lock lock(mutex);
        if(now/1000!=second) {
            if(dropped) GALLERY_DIAG("OG-ARRIVAL-DIAG event-overflow dropped={}",dropped);
            second=now/1000;count=0;dropped=0;
        }
        if(count++>=128) {++dropped;return RE::BSEventNotifyControl::kContinue;}
        GALLERY_DIAG("OG-ARRIVAL-DIAG equip-event tick={} actor={:08X} form={:08X} equipped={} original={:08X} uid={}",
            now,e->actor->GetFormID(),e->baseObject,e->equipped,e->originalRefr,e->uniqueID);
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* e,RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        if(DetailEnabled() && e && e->menuName=="Loading Menu") GALLERY_DIAG("OG-ARRIVAL-DIAG loading-menu tick={} opening={}",GetTickCount64(),e->opening);
        return RE::BSEventNotifyControl::kContinue;
    }
};
void RegisterFollowerArrivalDiagnostics() {
    static FollowerArrivalEvents sink;
    static bool registered=false;
    if(registered) return;
    auto* holder=RE::ScriptEventSourceHolder::GetSingleton();auto* ui=RE::UI::GetSingleton();
    if(!holder || !ui) return;
    holder->AddEventSink<RE::TESEquipEvent>(&sink);ui->AddEventSink<RE::MenuOpenCloseEvent>(&sink);registered=true;
    GALLERY_DIAG("OG-ARRIVAL-DIAG registered; read-only snapshots and events; event origin is not caller attribution");
}
void SnapshotFollowerArrival(RE::Actor* actor,FollowerAutoEntry& e) {
    if(!DetailEnabled()) {e.diagnosticSnapshot.clear();return;}
    const auto started=GetTickCount64();
    try {
        nlohmann::json j;
        const auto* base=actor->GetActorBase();const auto* outfit=base?base->defaultOutfit:nullptr;
        j["base"]=base?base->GetFormID():0;j["defaultOutfit"]=outfit?outfit->GetFormID():0;
        j["defaultForms"]=nlohmann::json::array();
        if(outfit) for(auto* form:outfit->outfitItems) if(form) j["defaultForms"].push_back(form->GetFormID());
        j["cell"]=actor->GetParentCell()?actor->GetParentCell()->GetFormID():0;
        j["name"]=e.name;j["armor"]=nlohmann::json::array();
        unsigned count=0;
        for(const auto& [obj,data]:actor->GetInventory()) {
            if(!obj || !obj->As<RE::TESObjectARMO>() || !data.second) continue;
            if(++count>256) continue;
            nlohmann::json item={{"form",obj->GetFormID()},{"count",data.first},{"worn",data.second->IsWorn()},
                {"name",std::string(obj->GetName()?obj->GetName():"").substr(0,96)}};
            item["outfitTags"]=nlohmann::json::array();
            if(data.second->extraLists) for(auto* extra:*data.second->extraLists) if(extra)
                if(auto* tag=extra->GetByType<RE::ExtraOutfitItem>()) item["outfitTags"].push_back(tag->id);
            j["armor"].push_back(std::move(item));
        }
        j["truncated"]=count>256;
        // Stable form ordering prevents pointer-map ordering from producing noise.
        auto& armor=j["armor"];std::sort(armor.begin(),armor.end(),[](const auto& a,const auto& b){return a.at("form")<b.at("form");});
        const auto dump=j.dump(-1,' ',false,nlohmann::json::error_handler_t::replace);
        if(dump!=e.diagnosticSnapshot) {
            GALLERY_DIAG("OG-ARRIVAL-DIAG snapshot tick={} actor={:08X} elapsedMs={} data={}",started,actor->GetFormID(),GetTickCount64()-started,dump);
            e.diagnosticSnapshot=dump;
        }
    }catch(const std::exception& ex) {
        // Diagnostic failures must never skip or alter normal outfit handling.
        const std::string failure=std::string("error:")+ex.what();
        if(e.diagnosticSnapshot!=failure) GALLERY_DIAG("OG-ARRIVAL-DIAG snapshot actor={:08X} error={}",actor->GetFormID(),ex.what());
        e.diagnosticSnapshot=failure;
    }
}
