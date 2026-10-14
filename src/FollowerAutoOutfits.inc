// Included after AutoOutfits.inc. Engine access is confined to the game thread.
struct FollowerAutoEntry {
    AutoOutfitSettings settings;
    unsigned revision{};
    AutoOutfit::Policy policy;
    AutoOutfit::StyleDebounce style;
    AutoOutfit::CombatSession combat;
    AutoOutfit::SceneSuspend scene;
    FollowerAuto::ArrivalCheck arrival;
    AutoEnvironmentDisplay environment;
    std::string status="Disabled", location, name;
    std::string diagnosticSnapshot; // diagnostic build only; not serialized
};
std::mutex followerAutoMutex;
std::map<std::uint32_t,FollowerAutoEntry> followerAutos;
std::atomic<bool> followerAutoAny{};
std::atomic<ULONGLONG> followerAutoDue{};
std::uint32_t followerAutoVerifying{}; // game thread; shared verification lane
unsigned followerAutoVerifyRevision{};
std::uint32_t followerAutoLastServed{};
FollowerAutoEntry GetFollowerAuto(std::uint32_t id) {
    std::scoped_lock lock(followerAutoMutex);
    const auto it=followerAutos.find(id); return it==followerAutos.end()?FollowerAutoEntry{}:it->second;
}
bool FollowerAutomaticOwns(std::uint32_t id) {
    std::scoped_lock lock(followerAutoMutex);
    const auto it=followerAutos.find(id);return it!=followerAutos.end() && it->second.settings.enabled;
}
bool SetFollowerAuto(std::uint32_t id,const AutoOutfitSettings& settings,bool rearm) {
    try {
        if(!id || id==0x14) throw std::runtime_error("Follower target unavailable");
        FollowerAuto::Validate(settings);
        std::scoped_lock lock(followerAutoMutex);
        if(!FollowerAuto::CanRegister(followerAutos.size(),followerAutos.contains(id))) throw std::runtime_error("Automatic outfits support up to two followers. Remove another follower's settings first.");
        auto& entry=followerAutos[id];
        {std::scoped_lock nameLock(statusMutex);entry.name=studioActorName;}
        if(rearm) {entry.policy={};entry.style={};}
        entry.settings=settings; ++entry.revision;
        if(!settings.enabled) entry.status="Disabled"; else if(settings.paused) entry.status="Paused";
        else entry.status="Waiting for normal play";
        followerAutoAny=true; followerAutoDue=0;
        return true;
    }catch(const std::exception& e) {SetStatus(e.what());return false;}
}
void RemoveFollowerAuto(std::uint32_t id) {
    std::scoped_lock lock(followerAutoMutex); followerAutos.erase(id); followerAutoAny=!followerAutos.empty();
}
void ClearFollowerAutoOutfits() {
    std::scoped_lock lock(followerAutoMutex); followerAutos.clear();followerAutoAny=false;followerAutoDue=0;
}
void ResetFollowerAutoRuntime() {
    followerAutoVerifying=0; followerAutoLastServed=0; followerAutoDue=0; equipmentLaneDue=0;
    std::scoped_lock lock(followerAutoMutex);
    for(auto& [id,e]:followerAutos) {const auto settings=e.settings;const auto revision=e.revision+1;e={};e.settings=settings;e.revision=revision;}
}
void SaveFollowerAutoOutfits(SKSE::SerializationInterface* s) {
    std::map<std::uint32_t,FollowerAutoEntry> copy;
    {std::scoped_lock lock(followerAutoMutex);copy=followerAutos;}
    for(const auto& [id,e]:copy) {
        auto json=nlohmann::json::parse(FollowerAuto::Encode(e.settings));json["actorName"]=e.name.substr(0,256);
        const auto bytes=json.dump(-1,' ',false,nlohmann::json::error_handler_t::replace);
        if(!s->OpenRecord(FollowerAuto::Record,1) || !s->WriteRecordData(id) || !s->WriteRecordData(bytes.data(),static_cast<std::uint32_t>(bytes.size())))
            SKSE::log::error("OG-FOLLOWER-AUTO save failed actor={:08X}",id);
    }
}
bool LoadFollowerAutoOutfit(SKSE::SerializationInterface* s,std::uint32_t type,std::uint32_t version,std::uint32_t length) {
    if(type!=FollowerAuto::Record) return false;
    if(version!=1 || length<=4 || length>8196) {SKSE::log::warn("OG-FOLLOWER-AUTO invalid record header");return true;}
    std::uint32_t old{},id{};
    if(s->ReadRecordData(old)!=sizeof(old)) return true;
    std::string bytes(length-4,'\0');
    if(s->ReadRecordData(bytes.data(),static_cast<std::uint32_t>(bytes.size()))!=bytes.size()) return true;
    try {
        FollowerAutoEntry e;e.settings=FollowerAuto::Decode(bytes);
        e.name=nlohmann::json::parse(bytes).value("actorName",std::string{});if(e.name.size()>256) e.name.resize(256);
        if(!s->ResolveFormID(old,id) || !id || id==0x14) throw std::runtime_error("Unresolved follower");
        std::scoped_lock lock(followerAutoMutex);
        if(followerAutos.contains(id) || followerAutos.size()>=FollowerAuto::Limit) throw std::runtime_error("Duplicate or excess follower");
        followerAutos.emplace(id,std::move(e));followerAutoAny=true;
    }catch(const std::exception& e) {SKSE::log::warn("OG-FOLLOWER-AUTO skipped record: {}",e.what());}
    return true;
}
bool FollowerAutoDue() {return followerAutoAny && GetTickCount64()>=followerAutoDue.load();}
bool FollowerAutoVerificationDone(bool ok) {
    if(!followerAutoVerifying) return false;
    std::scoped_lock lock(followerAutoMutex);
    const auto it=followerAutos.find(followerAutoVerifying);
    if(it!=followerAutos.end() && it->second.revision==followerAutoVerifyRevision) {
        if(!ok) it->second.policy.lastPhoto.clear();
        it->second.status=ok?"Outfit verified":"Equipment differs: check log";
    }
    SKSE::log::info("OG-FOLLOWER-AUTO verified actor={:08X} success={}",followerAutoVerifying,ok);
    followerAutoVerifying=0;return true;
}
bool FollowerAutoSceneBlocked(RE::Actor* actor) {
    return ostimSupported && actor && actor->IsInFaction(ostimSceneFaction);
}
bool FollowerAutoVerificationBlocked(RE::Actor* actor) {return followerAutoVerifying && FollowerAutoSceneBlocked(actor);}
void FollowerAutoManualChange(RE::Actor* actor) {
    if(!actor) return;
    const auto id=actor->GetFormID();auto e=GetFollowerAuto(id);
    if(!e.settings.enabled) return;
    const auto where=ClassifyAutoPlace(actor);const auto env=ReadAutoEnvironment(actor);
    if(actor->IsInCombat()) e.combat.Manual(e.policy);
    else {e.combat.Observe(false,e.policy); e.policy.Manual(static_cast<int>(AutoOutfit::SelectCondition(where.place,env.flags,false,e.settings.slots,where.guild)));}
    std::scoped_lock lock(followerAutoMutex);
    if(auto it=followerAutos.find(id);it!=followerAutos.end() && it->second.revision==e.revision) {it->second.policy=e.policy;it->second.combat=e.combat;it->second.arrival.Cancel();}
}
#include "FollowerArrivalDiagnostics.inc"
void MaintainFollowerAutoOutfits() {
    if(!FollowerAutoDue()) return;
    const auto now=GetTickCount64();followerAutoDue=now+1000;
    std::vector<std::pair<std::uint32_t,FollowerAutoEntry>> entries;
    {std::scoped_lock lock(followerAutoMutex);for(const auto& e:followerAutos) entries.push_back(e);}
    // Round-robin prevents a short-interval follower from always winning the lane.
    if(entries.size()==2 && entries.front().first==followerAutoLastServed) std::swap(entries[0],entries[1]);
    for(auto& [id,e]:entries) {
        const auto oldStatus=e.status;
        const auto oldLocation=e.location;
        const auto oldSlot=e.environment.selectedSlot;
        auto evaluate=[&] {
            auto block=[&](const char* reason) {e.policy.Block();e.style.Block();e.arrival.Block();e.status=reason;};
            auto* actor=RE::TESForm::LookupByID<RE::Actor>(id);
            if(!e.settings.enabled) {block("Disabled");return;}
            // Automatic and fixed maintenance must never fight over this actor.
            if(actor) ReleaseFollowerOutfit(actor);
            auto* player=RE::PlayerCharacter::GetSingleton();auto* ui=RE::UI::GetSingleton();auto* camera=RE::PlayerCamera::GetSingleton();
            auto* cell=actor?actor->GetParentCell():nullptr;
            e.arrival.Observe(cell?cell->GetFormID():0,actor && actor->Get3D() && cell && !actor->IsDisabled() && !actor->IsDeleted(),ui && ui->IsMenuOpen("Loading Menu"));
            if(!actor || !actor->Get3D() || actor->IsDisabled() || actor->IsDeleted() || !actor->GetParentCell()) {block("Follower is not loaded or is disabled.");return;}
            if(!IsCurrentOutfitFollower(actor) || !actor->HasKeywordString("ActorTypeNPC")) {block("Follower is no longer accompanying you.");return;}
            e.name=actor->GetName()?actor->GetName():"";
            SnapshotFollowerArrival(actor,e);
            const auto where=ClassifyAutoPlace(actor);const auto env=ReadAutoEnvironment(actor);
            const auto& slots=e.settings.slots;const bool fighting=actor->IsInCombat();
            const bool styled=fighting && AutoOutfit::HasCombatStyles(slots);
            const auto condition=styled?ReadCombatStyle(actor):AutoOutfit::SelectCondition(where.place,env.flags,fighting,slots,where.guild);
            const int place=static_cast<int>(condition);
            e.location=where.name;e.environment={env,static_cast<int>(AutoOutfit::IsGuild(where.guild)?where.guild:where.place),AutoOutfit::ChooseSlot(condition,slots,where.town)};
            if(e.combat.Observe(fighting,e.policy)) e.style.Block();
            const auto scene=e.scene.Observe(ostimInstalled,ostimSupported,FollowerAutoSceneBlocked(actor),now,static_cast<std::uint64_t>(e.settings.ostimRecoverySeconds)*1000);
            if(e.settings.paused) {block("Paused");return;}
            if(AutoOutfit::SceneBlocks(scene)) {block(OStimStateLabel(scene));return;}
            const auto* state=actor->AsActorState();
            if(!player || !ui || !camera || active || saveBusy || craftActive ||
                ui->GameIsPaused() || ui->numItemMenus || ui->numApplicationMenus || ui->IsMenuOpen("Loading Menu") || ui->IsMenuOpen("Main Menu") ||
                ui->IsMenuOpen("Dialogue Menu") || ui->IsMenuOpen("Console") || ui->IsMenuOpen("PhotoMode") ||
                !(camera->IsInFirstPerson() || camera->IsInThirdPerson()) || actor->IsDead() || actor->IsInKillMove() || actor->IsInRagdollState() ||
                state->IsUnconscious() || state->IsStaggered() || actor->IsOnMount() || actor->IsInMidair() ||
                !AutoOutfit::SwimmingChangeAllowed(state->IsSwimming(),condition,slots)) {block("Waiting for normal play");return;}
            if(!fighting && !env.ready) {block("Waiting for environment data");return;}
            if(styled) {
                const auto weapon=state->GetWeaponState();
                if(!actor->GetActorRuntimeData().currentProcess || (weapon!=RE::WEAPON_STATE::kDrawn && weapon!=RE::WEAPON_STATE::kSheathed)) {block("Waiting for stable combat equipment");return;}
                if(!e.style.Observe(condition,now)) {e.policy.Block();e.status="Waiting for stable combat equipment";return;}
            }
            if(fighting && !e.combat.Allowed(e.settings.combatEnabled,AutoOutfit::ChooseSlot(condition,slots,false)>=0,styled?place:-1)) {
                e.policy.Block();e.status=e.combat.manualOverride?"Manual outfit until combat ends":e.combat.attempted?"Combat outfit already handled":"Combat outfits disabled; waiting for combat to end";return;
            }
            if(actor->IsAttacking() || CombatCastingBusy(actor)) {e.policy.Block();e.arrival.Block();e.status="Waiting for attack or casting to finish";return;}
            const auto delay=AutoOutfit::ConditionDelay(condition,e.settings.delaySeconds*1000,e.settings.sneakDelaySeconds*1000);
            const bool ordinary=e.policy.Observe(place,now,delay,e.settings.combatIntervalSeconds*1000);
            if(e.policy.manual==place || e.combat.manualOverride) e.arrival.Cancel();
            // Preserve normal stability and the five-second noncombat interval.
            // Arrival repair is restricted to noncombat, previously handled work.
            if(fighting) e.arrival.Block();
            const bool arrivalReady=!fighting && e.arrival.Ready(now);
            const auto photo=AutoOutfit::Choose(condition,slots,where.town);
            const bool early=!fighting && e.policy.manual!=place &&
                e.arrival.EarlyReady(e.policy.handled,e.policy.stable,place,e.policy.lastPhoto,photo) &&
                (!e.policy.lastAttempt || now-e.policy.lastAttempt>=5000);
            const bool delayed=!fighting && arrivalReady && e.policy.handled && e.policy.manual!=place &&
                e.policy.candidate==place && now-e.policy.since>=delay &&
                (!e.policy.lastAttempt || now-e.policy.lastAttempt>=5000);
            const bool recheck=early || delayed;
            if(!ordinary && !recheck) {
                if(e.policy.manual==place) e.status="Manual outfit until the selected condition changes";
                else if(!e.policy.handled) e.status="Waiting for a stable condition";
                else if(!fighting) e.status="Already applied";
                return;
            }
            if(!FollowerAuto::MayApply(now,equipmentLaneDue,verificationTicks.load()!=0)) {e.status="Waiting for another outfit change";return;}
            if(recheck) {
                if(early) e.arrival.EarlyChecked();
                else e.arrival.Checked();
                if(photo.empty()) {e.arrival.Cancel();e.status="No outfit assigned";return;}
            }
            if(fighting) e.combat.Consume(styled?place:-1);
            if(!recheck && !e.policy.Attempt(photo,now)) {e.status=photo.empty()?"No outfit assigned":"Already applied";return;}
            try {
                Preset p;bool found=false;
                {std::scoped_lock lock(presetsMutex);for(const auto& candidate:presets) if(candidate.photo==photo) {p=candidate;found=true;break;}}
                if(!found) throw std::runtime_error("Assigned preset not found");
                if(p.slotMask || p.accessories || p.exchangeSlots) throw std::runtime_error("Assign a full outfit without slot overrides");
                const auto file=std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Library.json");
                if(std::filesystem::exists(file)) {
                    const auto labels=ReadLibrary(file);const auto it=labels.labels.find(photo);
                    if(it!=labels.labels.end()) {
                        if(it->second.deleted) throw std::runtime_error("Assigned preset is in Trash");
                        if(it->second.exchangeSlots) throw std::runtime_error("Assign a full outfit without slot overrides");
                        p.outfitItems=it->second.outfitItems;if(!it->second.name.empty()) p.name=it->second.name;
                    }
                }
                if(p.outfitItems) p=FilterExchangeSlots(p,{});
                p=ClothingPreset(p); // follower automation never changes weapons/spells
                (void)ResolvePresetSlots(p);
                if(p.items.empty()) throw std::runtime_error("Assigned outfit has no armor");
                bool same=false;try {same=SameEquipment(p,SnapshotEquipment(actor));} catch(const std::exception&) {if(recheck) throw;}
                if(recheck) GALLERY_DIAG("OG-FOLLOWER-AUTO arrival-check actor={:08X} cell={:08X} same={} photo={} check={} early={}",id,cell->GetFormID(),same,photo,e.arrival.checks,early);
                if(same) {e.policy.Applied(photo);e.status="Already wearing assigned outfit";return;}
                if(recheck) {
                    if(!early) e.arrival.Cancel(); // retain bounded delayed checks after early repair
                    e.policy.lastAttempt=now;
                }
                equipmentLaneDue=now+1000;followerAutoLastServed=id;
                e.policy.lastPhoto.clear();
                const bool prefer=preferEnchanted.load();
                const auto result=ApplyEquipment(actor,p,true,prefer);
                verificationActor=actor->CreateRefHandle();verificationTarget=p;verificationPrefer=prefer;
                verificationDue=GetTickCount64()+750;verificationTicks=1;autoVerifying=false;
                followerAutoVerifying=id;followerAutoVerifyRevision=e.revision;
                e.policy.Applied(photo);e.status="Verifying outfit";
                SKSE::log::info("OG-FOLLOWER-AUTO apply actor={:08X} photo={} condition={} result={}",id,photo,place,result);
            }catch(const std::exception& ex) {e.status="Skipped: check log";SKSE::log::warn("OG-FOLLOWER-AUTO skip actor={:08X} photo={} reason={}",id,photo,ex.what());}
        };
        evaluate();
        if(e.status!=oldStatus || e.location!=oldLocation || e.environment.selectedSlot!=oldSlot)
            SKSE::log::info("OG-FOLLOWER-AUTO actor={:08X} status={} location={} slot={} candidate={} stable={} handled={} manual={} lastPhoto={} verifying={} laneWaitMs={}",
                id,e.status,e.location,e.environment.selectedSlot,e.policy.candidate,e.policy.stable,e.policy.handled,e.policy.manual,e.policy.lastPhoto,
                verificationTicks.load()!=0,equipmentLaneDue>now?equipmentLaneDue-now:0);
        std::scoped_lock lock(followerAutoMutex);
        if(auto it=followerAutos.find(id);it!=followerAutos.end() && it->second.revision==e.revision) it->second=std::move(e);
    }
}
