// UI-owned mode/cache. Only value snapshots cross the one-shot game task.
bool missingCheckMode{}, missingCheckPending{}, missingCheckFocused{};
unsigned missingOrderSerial{}, missingFlaggedCount{};
MissingPresetResults missingResults;
std::string missingCheckError;
std::atomic<unsigned> missingCheckGeneration{};
std::atomic<bool> missingResponseReady{};
std::mutex missingResponseMutex;
struct MissingResponse {unsigned generation{}, session{}; MissingPresetResults results; std::string error;};
std::optional<MissingResponse> missingResponse;
void SetMissingCheckMode(bool enabled) {
    const unsigned generation=++missingCheckGeneration;
    missingCheckMode=enabled; missingCheckPending=enabled;
    missingResults.clear(); missingCheckError.clear(); missingFlaggedCount=0; ++missingOrderSerial;
    {std::scoped_lock lock(missingResponseMutex); missingResponse.reset(); missingResponseReady=false;}
    if(!enabled) return;
    std::vector<Preset> snapshot;
    {std::scoped_lock lock(presetsMutex); snapshot=presets;}
    const unsigned session=epoch.load(), opening=studioOpenSerial.load();
    SKSE::GetTaskInterface()->AddTask([generation,session,opening,snapshot=std::move(snapshot)]() {
        const auto current=[&]{return generation==missingCheckGeneration && session==epoch && opening==studioOpenSerial && ready && active;};
        if(!current()) return;
        MissingResponse response; response.generation=generation; response.session=session;
        const auto start=GetTickCount64(); unsigned lookups=0;
        try {
            auto* handler=RE::TESDataHandler::GetSingleton();
            if(!handler) throw std::runtime_error("Equipment manager unavailable");
            response.results=CheckMissingPresetData(snapshot,[&](const Item& item) {
                if(!current()) throw std::runtime_error("Cancelled");
                ++lookups;
                auto* form=ResolveItem(item);
                return form && form->As<RE::TESObjectARMO>();
            });
        } catch(const std::exception& e) {response.results.clear(); response.error=e.what();}
        if(!current()) return;
        unsigned flagged=0; for(const auto& entry:response.results) if(!entry.second.empty()) ++flagged;
        SKSE::log::info("MISSING-CHECK1 presets={} uniqueLookups={} flagged={} elapsed_ms={} error={}",snapshot.size(),lookups,flagged,GetTickCount64()-start,response.error);
        std::scoped_lock lock(missingResponseMutex);
        if(!current()) return;
        missingResponse=std::move(response); missingResponseReady=true;
    });
}
void ReceiveMissingCheck() {
    if(!missingResponseReady.load()) return;
    std::optional<MissingResponse> response;
    {std::scoped_lock lock(missingResponseMutex); response=std::move(missingResponse); missingResponse.reset(); missingResponseReady=false;}
    if(!response || !missingCheckMode || response->generation!=missingCheckGeneration || response->session!=epoch) return;
    missingResults=std::move(response->results); missingCheckError=std::move(response->error);
    missingFlaggedCount=0; for(const auto& entry:missingResults) if(!entry.second.empty()) ++missingFlaggedCount;
    missingCheckPending=false; ++missingOrderSerial;
}
