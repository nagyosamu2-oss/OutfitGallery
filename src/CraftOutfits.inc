// Game-thread session state. Event callbacks retain handles/values only.
Craft::Session craft;
RE::ObjectRefHandle craftFurniture;
std::atomic<unsigned> craftMenuSerial{};
Craft::Kind CraftFurnitureKind(RE::TESObjectREFR* ref) {
    auto* furniture=ref && ref->GetBaseObject()?ref->GetBaseObject()->As<RE::TESFurniture>():nullptr;
    if(!furniture) return Craft::Kind::None;
    using Bench=RE::TESFurniture::WorkBenchData::BenchType;
    const auto& data=furniture->workBenchData;
    if(data.benchType==Bench::kSmithingWeapon || data.benchType==Bench::kSmithingArmor ||
        (data.benchType==Bench::kCreateObject && data.usesSkill==RE::ActorValue::kSmithing)) return Craft::Kind::Smithing;
    if(data.benchType==Bench::kAlchemy || data.benchType==Bench::kAlchemyExperiment) return Craft::Kind::Alchemy;
    if(data.benchType==Bench::kEnchanting || data.benchType==Bench::kEnchantingExperiment) return Craft::Kind::Enchanting;
    return Craft::Kind::None;
}
void ResetCraft() {craft={};craftFurniture={};craftActive=false;}
// Diagnostic logging must not query ActorValueOwner via Actor's compile-time base
// layout. AE 1.6.629+ relocates this subobject; the original direct virtual call
// crashed on 1.6.1170 before ApplyEquipment. Keep this logger engine-independent.
void LogCraftPhase(const char* phase) {
    SKSE::log::info("OG-CRAFT {} kind={} target={:08X} attempted={} changed={} applied={}",
        phase,Craft::Name(craft.kind),craft.target,craft.attempted,craft.changed,craft.applied);
}

bool MaintainCraft(RE::PlayerCharacter* player,RE::UI* ui,ULONGLONG now) {
    if(!craft.target) return false;
    const int slot=Craft::SlotFor(craft.kind);
    if(slot<0) {ResetCraft();return false;}
    const auto target=craftFurniture.get();
    const auto occupied=player->GetOccupiedFurniture().get();
    const bool same=target && occupied && target.get()==occupied.get();
    const bool menu=ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME);
    const auto* state=player->AsActorState();
    if(craft.Finished(now,same,state->GetSitSleepState()==RE::SIT_SLEEP_STATE::kNormal,menu)) {
        SKSE::log::info("OG-CRAFT finished target={:08X} changed={} occupied={} expired={}",craft.target,craft.changed,craft.occupied,now-craft.started>=10000);
        // Only a real mutation needs restoration. A skipped attempt preserves manual choices.
        if(craft.changed) {autoPolicy={};autoStyle.Block();autoOutcome="Ready";}
        ResetCraft();return false;
    }
    std::string photo;{std::scoped_lock lock(autoMutex);photo=autoSlots[slot];}
    const bool scene=OStimBlocksAutomatic(player);
    const bool allowed=target && CraftFurnitureKind(target.get())==craft.kind && autoEnabled && !autoPaused && !photo.empty() &&
        !scene && !active && !editPending && !saveBusy && !verificationTicks && !player->IsInCombat() &&
        !player->IsDead() && !player->IsDisabled() && !player->IsInKillMove() && !player->IsInRagdollState() &&
        !state->IsUnconscious() && !state->IsStaggered() && !state->IsSwimming() && !player->IsOnMount() &&
        !player->IsAttacking() && !CombatCastingBusy(player) && !ui->GameIsPaused() && !ui->numItemMenus &&
        !ui->numApplicationMenus && !ui->IsMenuOpen("Dialogue Menu") && !ui->IsMenuOpen("Console") && !ui->IsMenuOpen("PhotoMode");
    const bool wasAttempted=craft.attempted;
    if(craft.Try(same,menu,craftMenuSerial.load(),allowed)) {
        try {
            Preset p;bool found=false;
            {std::scoped_lock lock(presetsMutex);for(const auto& entry:presets) if(entry.photo==photo){p=entry;found=true;break;}}
            if(!found) throw std::runtime_error("Assigned preset not found");
            if(p.slotMask || p.accessories || p.exchangeSlots) throw std::runtime_error("Assign a full outfit without slot overrides");
            const auto file=std::filesystem::path("Data/SKSE/Plugins/OutfitGallery/Library.json");
            if(std::filesystem::exists(file)) {
                const auto labels=ReadLibrary(file);const auto it=labels.labels.find(photo);
                if(it!=labels.labels.end() && (it->second.deleted || it->second.exchangeSlots))
                    throw std::runtime_error("Assigned preset is deleted or has slot overrides");
                if(it!=labels.labels.end()) p.outfitItems=it->second.outfitItems;
            }
            if(p.outfitItems) p=FilterExchangeSlots(p,{});
            (void)ResolvePresetSlots(p);
            if(ClothingPreset(p).items.empty()) throw std::runtime_error("Assigned outfit has no armor");
            // Recheck after disk work. A menu that opened and closed also invalidates the attempt.
            if(ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME) || craftMenuSerial.load()!=craft.menuSerial || OStimBlocksAutomatic(player))
                throw std::runtime_error("Menu or scene started before equipment change");
            LogCraftPhase("before-apply");
            // Generate base items as in ordinary auto outfits, but stop at the menu deadline.
            // No delayed verification/reclamation is scheduled inside crafting menus.
            // Mark first: a partial engine failure must still permit ordinary restoration on exit.
            craft.changed=true;
            const auto result=ApplyEquipment(player,p,true,preferEnchanted.load(),[&] {
                return craftMenuSerial.load()==craft.menuSerial &&
                    !ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME) && !OStimBlocksAutomatic(player);
            });
            craft.applied=true;
            autoPolicy.Block();autoStyle.Block();
            LogCraftPhase("after-apply");
            SKSE::log::info("OG-CRAFT apply photo={} elapsedFromActivationMs={} result={}",photo,GetTickCount64()-craft.started,result);
            SetStatus(result);
        }catch(const std::exception& e){
            SKSE::log::warn("OG-CRAFT skipped photo={} reason={}; no retry this session",photo,e.what());
            SetStatus(std::string(Craft::Name(craft.kind))+": "+e.what());
        }
    } else if(!wasAttempted && craft.attempted) {
        SKSE::log::info("OG-CRAFT skipped target={:08X} menuOpen={} menuAdvanced={} allowed={} sameFurniture={}; no retry this session",
            craft.target,menu,craftMenuSerial.load()!=craft.menuSerial,allowed,same);
    }
    const bool enchanting=craft.kind==Craft::Kind::Enchanting;
    const bool alchemy=craft.kind==Craft::Kind::Alchemy;
    AutoStatus(craft.applied?(enchanting?"Enchanting outfit applied; waiting for exit":alchemy?"Alchemy outfit applied; waiting for exit":"Smithing outfit applied; waiting for exit"):
        craft.attempted?(enchanting?"Enchanting skipped: check log":alchemy?"Alchemy skipped: check log":"Smithing skipped: check log"):
        (enchanting?"Waiting for enchanting entry":alchemy?"Waiting for alchemy entry":"Waiting for smithing entry"),slot,"");
    {std::scoped_lock lock(autoMutex);autoEnvironment.selectedSlot=slot;}
    return true;
}
void QueueCraftFurniture(RE::TESObjectREFR* actor,RE::TESObjectREFR* target,bool exiting,const char* eventName) {
    if(!actor || actor!=RE::PlayerCharacter::GetSingleton() || !target || !ready) return;
    const auto handle=target->CreateRefHandle();const auto generation=epoch.load();
    const auto serial=craftMenuSerial.load();const auto stamp=GetTickCount64();
    if(auto* tasks=SKSE::GetTaskInterface()) tasks->AddTask([handle,generation,serial,stamp,exiting,eventName] {
        if(generation!=epoch.load() || !ready) return;
        const auto ref=handle.get();auto* player=RE::PlayerCharacter::GetSingleton();auto* ui=RE::UI::GetSingleton();
        if(!ref || !player || !player->Get3D() || !ui || ui->IsMenuOpen("Loading Menu") || ui->IsMenuOpen("Main Menu")) return;
        if(exiting) {
            if(craft.target==ref->GetFormID()) {craft.exited=true;SKSE::log::info("OG-CRAFT furniture-exit target={:08X}",craft.target);}
            return;
        }
        const auto kind=CraftFurnitureKind(ref.get());
        const int slot=Craft::SlotFor(kind);
        if(slot<0) return;
        if(!craft.target) {
            if(!autoEnabled || autoPaused) return;
            {std::scoped_lock lock(autoMutex);if(autoSlots[slot].empty()) return;}
            if(!craft.Begin(ref->GetFormID(),stamp,serial,kind)) return;
            craftFurniture=handle;craftActive=true;
        }
        if(craft.target!=ref->GetFormID()) return;
        SKSE::log::info("OG-CRAFT event={} kind={} target={:08X} queuedMs={} menuOpen={}",eventName,Craft::Name(craft.kind),craft.target,GetTickCount64()-stamp,ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME));
        MaintainCraft(player,ui,GetTickCount64());
    });
}
struct CraftEvents final:RE::BSTEventSink<RE::TESActivateEvent>,RE::BSTEventSink<RE::TESFurnitureEvent>,RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* e,RE::BSTEventSource<RE::TESActivateEvent>*) override {
        if(e) QueueCraftFurniture(e->actionRef.get(),e->objectActivated.get(),false,"activate");
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESFurnitureEvent* e,RE::BSTEventSource<RE::TESFurnitureEvent>*) override {
        if(e) QueueCraftFurniture(e->actor.get(),e->targetFurniture.get(),e->type==RE::TESFurnitureEvent::FurnitureEventType::kExit,"furniture-enter");
        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* e,RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        if(e && e->opening && e->menuName==RE::CraftingMenu::MENU_NAME) {
            ++craftMenuSerial;const auto generation=epoch.load();
            if(auto* tasks=SKSE::GetTaskInterface()) tasks->AddTask([generation] {
                if(generation!=epoch.load() || !ready || !craft.target) return;
                LogCraftPhase("menu-open-observed");
                // Latch the deadline even if another mod immediately closes the menu.
                craft.attempted=true;
            });
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
void RegisterCraftEvents() {
    static CraftEvents sink;static bool registered=false;
    if(registered) return;
    auto* holder=RE::ScriptEventSourceHolder::GetSingleton();auto* ui=RE::UI::GetSingleton();
    if(!holder || !ui) return;
    holder->AddEventSink<RE::TESActivateEvent>(&sink);
    holder->AddEventSink<RE::TESFurnitureEvent>(&sink);
    ui->AddEventSink<RE::MenuOpenCloseEvent>(&sink);registered=true;
    SKSE::log::info("OG-CRAFT smithing/alchemy/enchanting trial events registered; each empty assignment disables use");
}
