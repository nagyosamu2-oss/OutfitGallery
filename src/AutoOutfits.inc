// Included in Gallery before Tick. Engine state is read only on the game thread.
std::atomic<bool> autoEnabled{}, autoPaused{}, autoPanelVisible{}, autoCombatEnabled{}; // persisted preferences; transition state remains session-only
bool autoPanel{}; // renderer only
std::string autoSelectedPhoto; // renderer only
std::mutex autoMutex;
AutoOutfitSlots autoSlots;
std::atomic<int> autoDelaySeconds{3};
std::atomic<int> autoCombatIntervalSeconds{5};
std::atomic<int> autoSneakDelaySeconds{2};
bool autoHideAssignButtons=false; // renderer only after initialization
int autoColumns=3; // renderer only after initialization
bool autoSlotsWritable=true;
std::atomic<unsigned> autoRevision{};
std::atomic<ULONGLONG> autoDue{};
const std::filesystem::path autoFile="Data/SKSE/Plugins/OutfitGallery/AutoOutfits.json";
std::string autoState="Disabled", autoLocation;
int autoPlace{};
struct AutoEnvironmentInfo {AutoOutfit::Environment flags; float hour{12.f}; bool ready{};};
struct AutoEnvironmentDisplay {AutoEnvironmentInfo environment; int locationPlace{}, selectedSlot{-1};};
AutoEnvironmentDisplay autoEnvironment; // autoMutex, values only; no engine pointers in UI
AutoEnvironmentInfo ReadAutoEnvironment(RE::Actor* player) {
    AutoEnvironmentInfo info;
    auto* cell=player->GetParentCell();auto* sky=RE::Sky::GetSingleton();auto* calendar=RE::Calendar::GetSingleton();
    info.flags.outside=cell && !cell->IsInteriorCell();
    info.flags.swimming=player->AsActorState()->IsSwimming();
    info.flags.sneaking=player->IsSneaking();
    if(calendar) {info.hour=calendar->GetHour();info.flags.night=AutoOutfit::IsNight(info.hour);}
    info.ready=cell && calendar && info.hour>=0.f && info.hour<24.f && (!info.flags.outside || sky);
    if(info.flags.outside && sky) {info.flags.rain=sky->IsRaining();info.flags.snow=sky->IsSnowing();}
    return info;
}
void AutoEnvironmentStatus(const AutoEnvironmentInfo& info,int location,int slot) {
    std::scoped_lock lock(autoMutex);
    const auto& old=autoEnvironment.environment;
    if(old.flags.outside!=info.flags.outside || old.flags.night!=info.flags.night || old.flags.rain!=info.flags.rain ||
       old.flags.snow!=info.flags.snow || old.flags.swimming!=info.flags.swimming || old.flags.sneaking!=info.flags.sneaking || old.ready!=info.ready || autoEnvironment.locationPlace!=location || autoEnvironment.selectedSlot!=slot)
        SKSE::log::info("OG-AUTO environment outside={} hour={} night={} rain={} snow={} swimming={} ready={} locationPlace={} selectedSlot={}",info.flags.outside,info.hour,info.flags.night,info.flags.rain,info.flags.snow,info.flags.swimming,info.ready,location,slot);
    autoEnvironment={info,location,slot};
}
std::string autoOutcome="Ready"; // game thread only
AutoOutfit::Policy autoPolicy; // game thread only
AutoOutfit::StyleDebounce autoStyle; // game thread only
AutoOutfit::CombatSession autoCombat; // game thread only
unsigned autoSeenRevision{};
void AutoStatus(const std::string& text,int place,const std::string& location) {
    std::scoped_lock lock(autoMutex);
    if(autoState!=text || autoPlace!=place || autoLocation!=location)
        SKSE::log::info("OG-AUTO status={} place={} location={}",text,place,location);
    autoState=text; autoPlace=place; autoLocation=location;
}
void AutoVerificationDone(bool ok) {
    if(FollowerAutoVerificationDone(ok)) return;
    if(!autoVerifying) return;
    autoVerifying=false;
    if(!ok) autoPolicy.lastPhoto.clear();
    autoOutcome=ok?"Outfit verified":"Equipment differs: check log";
    std::scoped_lock lock(autoMutex);
    autoState=autoOutcome;
    SKSE::log::info("OG-AUTO verified={}",ok);
}
std::array<RE::BGSLocation*,AutoOutfit::GuildLocations.size()> guildLocations{}; // resolved once, game thread only
std::array<RE::TESObjectCELL*,AutoOutfit::GuildLocations.size()> guildCells{};
void InitializeGuildLocations() {
    auto* data=RE::TESDataHandler::GetSingleton();
    for(std::size_t i=0;i<guildLocations.size();++i) {
        const auto& rule=AutoOutfit::GuildLocations[i];
        auto* location=FindActivePlugin(data,rule.plugin)?data->LookupForm<RE::BGSLocation>(rule.localID,rule.plugin):nullptr;
        guildLocations[i]=location && !location->IsDeleted()?location:nullptr;
        auto* requiredCell=rule.requiredCell && FindActivePlugin(data,rule.plugin)?data->LookupForm<RE::TESObjectCELL>(rule.requiredCell,rule.plugin):nullptr;
        guildCells[i]=requiredCell && !requiredCell->IsDeleted()?requiredCell:nullptr;
        SKSE::log::info("OG-AUTO guild location plugin={} localID={:06X} slot={} resolved={:08X}",rule.plugin,rule.localID,static_cast<unsigned>(rule.place),guildLocations[i]?guildLocations[i]->GetFormID():0);
    }
}
struct AutoPlaceInfo { AutoOutfit::Place place=AutoOutfit::Place::Normal, guild=AutoOutfit::Place::Normal; bool town{}; std::string name; };
AutoPlaceInfo ClassifyAutoPlace(RE::Actor* player) {
    AutoPlaceInfo result;
    auto* loc=player->GetCurrentLocation();
    if(loc && loc->GetName()) result.name=loc->GetName();
    // Bounded parent traversal also supports interiors nested under a town.
    bool home=false, inn=false, dungeon=false, temple=false, shop=false;
    for(unsigned depth=0;loc && depth<32;++depth,loc=loc->parentLoc) {
        shop|=loc->HasKeywordString("LocTypeStore");
        home|=loc->HasKeywordString("LocTypePlayerHouse");
        inn|=loc->HasKeywordString("LocTypeInn");
        dungeon|=loc->HasKeywordString("LocTypeDungeon");
        temple|=loc->HasKeywordString("LocTypeTemple");
        result.town|=loc->HasKeywordString("LocTypeCity") || loc->HasKeywordString("LocTypeTown");
    }
    auto* cell=player->GetParentCell();
    const bool interior=cell && cell->IsInteriorCell();
    result.place=AutoOutfit::Classify(home,inn,result.town,dungeon,temple,interior,shop);
    result.guild=AutoOutfit::GuildForLocation(interior,player->GetCurrentLocation(),
        [](RE::BGSLocation* at){return at->parentLoc;},
        [cell](RE::BGSLocation* at){
            for(std::size_t i=0;i<guildLocations.size();++i)
                if(guildLocations[i] && at==guildLocations[i] && AutoOutfit::GuildCellAllowed(AutoOutfit::GuildLocations[i],guildCells[i] && cell==guildCells[i])) return AutoOutfit::GuildLocations[i].place;
            return AutoOutfit::Place::Normal;
        });
    return result;
}
void ResetCraft();
bool MaintainCraft(RE::PlayerCharacter*,RE::UI*,ULONGLONG);
std::atomic<bool> craftActive{};
void ResetAutoOutfits() {
    ResetCraft(); ResetFollowerAutoRuntime();
    // Preserve user preferences across loads; discard old-world transition history.
    autoCombat={}; autoStyle={}; autoPolicy={}; autoDue=0; autoVerifying=false; autoOutcome="Ready";
    ostimSuspend={};
    ostimSceneState=ostimSuspend.Observe(ostimInstalled,ostimSupported,false,GetTickCount64());
    {std::scoped_lock lock(autoMutex);autoEnvironment={};}
    AutoStatus(!autoEnabled?"Disabled":autoPaused?"Paused":"Waiting for normal play",0,"");
}
void AutoManualChange(RE::Actor* actor) {
    // A manual request owns the shared verification lane from this point.
    if(autoVerifying) AutoVerificationDone(false);
    (void)FollowerAutoVerificationDone(false);
    FollowerAutoManualChange(actor);
    if(actor && actor==RE::PlayerCharacter::GetSingleton()) {
        // Manual apply supersedes any pending automatic verification.
        autoVerifying=false;
        autoSeenRevision=autoRevision.load();
        if(actor->IsInCombat()) autoCombat.Manual(autoPolicy);
        else {
            autoCombat.Observe(false,autoPolicy);
            auto* player=RE::PlayerCharacter::GetSingleton();
            const auto where=ClassifyAutoPlace(player);const auto env=ReadAutoEnvironment(player);
            AutoOutfitSlots slots;{std::scoped_lock lock(autoMutex);slots=autoSlots;}
            autoPolicy.Manual(static_cast<int>(AutoOutfit::SelectCondition(where.place,env.flags,false,slots,where.guild)));
        }
        SKSE::log::info("OG-AUTO manual override condition={} combatConsumed={}",autoPolicy.manual,autoCombat.attempted);
    }
}
AutoOutfit::Hand ReadCombatHand(RE::TESForm* form) {
    if(!form) return AutoOutfit::Hand::Empty;
    if(auto* weapon=form->As<RE::TESObjectWEAP>()) {
        if(weapon->IsBow() || weapon->IsCrossbow()) return AutoOutfit::Hand::Archery;
        if(weapon->IsStaff()) return AutoOutfit::Hand::Magic;
        if(weapon->IsMelee()) return AutoOutfit::Hand::Melee;
    }
    if(form->As<RE::SpellItem>() || form->As<RE::ScrollItem>()) return AutoOutfit::Hand::Magic;
    return AutoOutfit::Hand::Other;
}
AutoOutfit::Place ReadCombatStyle(RE::Actor* player) {
    return AutoOutfit::CombatStyle(ReadCombatHand(player->GetEquippedObject(false)),ReadCombatHand(player->GetEquippedObject(true)));
}
bool CombatCastingBusy(RE::Actor* player) {
    for(auto source:{RE::MagicSystem::CastingSource::kLeftHand,RE::MagicSystem::CastingSource::kRightHand,RE::MagicSystem::CastingSource::kOther}) {
        const auto* caster=player->GetMagicCaster(source);
        if(caster && caster->state!=RE::MagicCaster::State::kNone) return true;
    }
    return false;
}
bool AutoDue() {return (craftActive || autoEnabled || (active && autoPanelVisible)) && GetTickCount64()>=autoDue.load();}
void MaintainAutoOutfits(bool beforeFollowers=false) {
    const auto now=GetTickCount64(); if(!FollowerAuto::PollPlayer(now,autoDue.load(),beforeFollowers)) return; autoDue=now+1000;
    auto* player=RE::PlayerCharacter::GetSingleton();
    auto* ui=RE::UI::GetSingleton(); auto* camera=RE::PlayerCamera::GetSingleton();
    if(!player || !player->Get3D() || !player->GetParentCell() || !ui || !camera || ui->IsMenuOpen("Loading Menu") || ui->IsMenuOpen("Main Menu")) {autoPolicy.Block();autoStyle.Block(); return;}
    if(MaintainCraft(player,ui,now)) return;
    const auto where=ClassifyAutoPlace(player);
    const bool fighting=player->IsInCombat();
    AutoOutfitSlots slots; {std::scoped_lock lock(autoMutex); slots=autoSlots;}
    const auto environment=ReadAutoEnvironment(player);
    const bool styled=fighting && AutoOutfit::HasCombatStyles(slots);
    const auto condition=styled?ReadCombatStyle(player):AutoOutfit::SelectCondition(where.place,environment.flags,fighting,slots,where.guild);
    AutoEnvironmentStatus(environment,static_cast<int>(AutoOutfit::IsGuild(where.guild)?where.guild:where.place),AutoOutfit::ChooseSlot(condition,slots,where.town));
    const int place=static_cast<int>(condition);
    if(autoCombat.Observe(fighting,autoPolicy)) {
        autoStyle.Block();
        SKSE::log::info("OG-AUTO combat={} enabled={} locationPlace={} location={}",fighting,autoCombatEnabled.load(),static_cast<int>(where.place),where.name);
    }
    if(autoSeenRevision!=autoRevision.load()) {autoSeenRevision=autoRevision.load(); autoPolicy={};autoStyle.Block();}
    const bool sceneBlocked=OStimBlocksAutomatic(player);
    if(!autoEnabled) {autoPolicy.Block();autoStyle.Block(); AutoStatus("Disabled",place,where.name); return;}
    if(autoPaused) {autoPolicy.Block();autoStyle.Block(); AutoStatus("Paused",place,where.name); return;}
    if(sceneBlocked) {autoPolicy.Block();autoStyle.Block();AutoStatus(OStimStateLabel(ostimSceneState.load()),place,where.name);return;}
    if(!fighting && !environment.ready) {autoPolicy.Block();autoStyle.Block();AutoStatus("Waiting for environment data",place,where.name);return;}
    auto* controls=RE::ControlMap::GetSingleton(); const auto* state=player->AsActorState();
    const bool blocked=active || saveBusy || ui->GameIsPaused() || ui->numItemMenus || ui->numApplicationMenus ||
        ui->IsMenuOpen("Dialogue Menu") || ui->IsMenuOpen("Console") || ui->IsMenuOpen("PhotoMode") ||
        player->IsDead() || player->IsDisabled() || player->IsInKillMove() || player->IsInRagdollState() || state->IsUnconscious() || state->IsStaggered() || player->IsOnMount() || !AutoOutfit::SwimmingChangeAllowed(state->IsSwimming(),condition,slots) ||
        state->GetSitSleepState()!=RE::SIT_SLEEP_STATE::kNormal ||
        !(camera->IsInFirstPerson() || camera->IsInThirdPerson()) || !controls ||
        !controls->IsMovementControlsEnabled() || !controls->IsActivateControlsEnabled();
    if(blocked) {autoPolicy.Block();autoStyle.Block(); AutoStatus("Waiting for normal play",place,where.name); return;}
    if(styled) {
        const auto weaponState=state->GetWeaponState();
        if(!player->GetActorRuntimeData().currentProcess ||
           (weaponState!=RE::WEAPON_STATE::kDrawn && weaponState!=RE::WEAPON_STATE::kSheathed)) {
            autoStyle.Block();autoPolicy.Block();AutoStatus("Waiting for stable combat equipment",place,where.name);return;
        }
        if(!autoStyle.Observe(condition,now)) {
            autoPolicy.Block();AutoStatus("Waiting for stable combat equipment",place,where.name);return;
        }
    }
    if(fighting && !autoCombat.Allowed(autoCombatEnabled.load(),AutoOutfit::ChooseSlot(condition,slots,false)>=0,styled?place:-1)) {
        autoPolicy.Block();
        AutoStatus(autoCombat.manualOverride?"Manual outfit until combat ends":autoCombat.attempted?"Combat outfit already handled":autoCombatEnabled?"No combat outfit assigned; waiting for combat to end":"Combat outfits disabled; waiting for combat to end",place,where.name);
        return;
    }
    if((styled || condition==AutoOutfit::Place::Sneaking) && (player->IsAttacking() || CombatCastingBusy(player))) {
        autoPolicy.Block();AutoStatus("Waiting for attack or casting to finish",place,where.name);return;
    }
    if(!autoPolicy.Observe(place,now,AutoOutfit::ConditionDelay(condition,static_cast<std::uint64_t>(autoDelaySeconds.load())*1000,static_cast<std::uint64_t>(autoSneakDelaySeconds.load())*1000),static_cast<std::uint64_t>(autoCombatIntervalSeconds.load())*1000)) {
        if(autoPolicy.manual==place) AutoStatus("Manual outfit until the selected condition changes",place,where.name);
        else if(!autoPolicy.handled) AutoStatus("Waiting for a stable condition",place,where.name);
        else AutoStatus(autoOutcome,place,where.name);
        return;
    }
    // A busy equipment lane is not a change in the player's condition. Keep
    // stability timers and do not consume the attempt until the lane is free.
    if(!FollowerAuto::MayApply(now,equipmentLaneDue,verificationTicks.load()!=0)) {
        AutoStatus("Waiting for another outfit change",place,where.name);return;
    }
    const auto photo=AutoOutfit::Choose(condition,slots,where.town);
    if(fighting) autoCombat.Consume(styled?place:-1); // Retry only after a different stable style or a new battle.
    if(!autoPolicy.Attempt(photo,now)) {autoOutcome=photo.empty()?"No outfit assigned":"Already applied"; AutoStatus(autoOutcome,place,where.name); return;}
    try {
        Preset p; bool found=false;
        {std::scoped_lock lock(presetsMutex); for(const auto& entry:presets) if(entry.photo==photo) {p=entry; found=true; break;}}
        if(!found) throw std::runtime_error("Assigned preset not found");
        if(p.slotMask || p.accessories || p.exchangeSlots) throw std::runtime_error("Assign a full outfit without slot overrides");
        const auto file=std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Library.json");
        if(std::filesystem::exists(file)) {
            const auto labels=ReadLibrary(file); auto it=labels.labels.find(photo);
            if(it!=labels.labels.end()) {
                if(it->second.deleted) throw std::runtime_error("Assigned preset is in Trash");
                if(it->second.exchangeSlots) throw std::runtime_error("Assign a full outfit without slot overrides");
                p.outfitItems=it->second.outfitItems;
                if(!it->second.name.empty()) p.name=it->second.name;
            }
        }
        // All records are resolved before ApplyEquipment can change the actor.
        if(p.outfitItems) p=FilterExchangeSlots(p,{});
            (void)ResolvePresetSlots(p);
        if(ClothingPreset(p).items.empty()) throw std::runtime_error("Assigned outfit has no armor");
        bool already=false;
        try {already=SameEquipment(ClothingPreset(p),SnapshotEquipment(player));} catch(const std::exception&) {}
        if(already) {autoPolicy.Applied(photo); autoOutcome="Already wearing assigned outfit"; AutoStatus(autoOutcome,place,where.name); return;}
        if(OStimBlocksAutomatic(player)) {
            autoPolicy.Block();autoStyle.Block();AutoStatus(OStimStateLabel(ostimSceneState.load()),place,where.name);return;
        }
        const auto start=GetTickCount64();
        enchantNotice.clear(); verificationPrefer=preferEnchanted.load();
        // A partial failure must not let the pre-combat photo cache suppress restoration.
        if(fighting) autoPolicy.lastPhoto.clear();
        equipmentLaneDue=now+1000;
        const auto result=ApplyEquipment(player,p,true,verificationPrefer);
        if(styled) {
            const auto after=ReadCombatStyle(player);
            if(after!=condition) {
                const int settled=static_cast<int>(after);
                autoCombat.Consume(settled);
                autoPolicy.stable=settled;autoPolicy.candidate=-1;autoPolicy.handled=true;
                autoStyle.Block();
                SKSE::log::info("OG-AUTO own outfit changed hand classification {} -> {}; suppress feedback",place,settled);
            }
        }
        const auto warning=result.find(" Registered enchantment not restored:");
        if(warning!=std::string::npos) enchantNotice=result.substr(warning);
        verificationActor=player->CreateRefHandle(); verificationTarget=ClothingPreset(p);
        verificationDue=GetTickCount64()+750; verificationTicks=1; autoVerifying=true;
        autoPolicy.Applied(photo);
        autoOutcome="Verifying outfit"; AutoStatus(autoOutcome,place,where.name);
        SKSE::log::info("OG-AUTO apply photo={} preset={} place={} elapsedMs={} result={}",photo,p.name,place,GetTickCount64()-start,result);
        SetStatus(result);
    } catch(const std::exception& e) {
        autoOutcome="Skipped: check log"; AutoStatus(autoOutcome,place,where.name);
        SKSE::log::warn("OG-AUTO skipped photo={} reason={}; no automatic retry until category/config changes",photo,e.what());
        SetStatus(std::string("Auto outfit: ")+e.what());
    }
}
