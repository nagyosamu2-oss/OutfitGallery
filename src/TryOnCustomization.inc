constexpr unsigned kOpenTryOnCustom = 38, kChangeTryOnCustom = 39, kPrepareTryOnItems = 40, kAcquireTryOnItems = 41;
struct TryOnCustomState {
    unsigned session{};
    bool ready{}, reset{};
    std::size_t action{};
    bool enabled{};
    std::string plugin, error;
    std::vector<Item> rows;
    std::vector<std::uint32_t> slots;
    std::vector<bool> selected, original, seen;
    Preset current;
};
struct TryOnInventoryState {
    unsigned session{};
    bool ready{}, obtaining{};
    std::size_t offset{};
    std::string plugin, error;
    std::vector<Item> rows;
};
TryOnCustomState tryOnCustom;
TryOnInventoryState tryOnInventory;
std::atomic<bool> tryOnCustomPending{}, tryOnInventoryPending{};
bool tryOnCustomVisible{}; // UI-thread only; this tool window does not dim the scene.

bool SameTryOnItems(const Preset& a, const Preset& b) {
    if (a.items.size() != b.items.size()) return false;
    for (std::size_t n = 0; n < a.items.size(); ++n)
        if (ExchangeItemKey(a.items[n]) != ExchangeItemKey(b.items[n])) return false;
    return true;
}

void ProcessTryOnCustomization(unsigned request, RE::Actor* actor) {
    if (request != kOpenTryOnCustom && request != kChangeTryOnCustom) return;
    TryOnCustomState state;
    { std::scoped_lock lock(randomTryOnMutex); state = tryOnCustom; }
    try {
        if (!actor || state.session != studioOpenSerial.load() || saveBusy || verificationTicks)
            throw std::runtime_error("Try-on session changed. Reopen customization.");
        auto current = GetTransmogTryOnPreset(actor);
        if (!current) throw std::runtime_error("Draw an outfit first.");
        if (request == kOpenTryOnCustom) {
            { std::scoped_lock lock(randomTryOnMutex);
              if (randomTryOnOriginSession == state.session) state.plugin = randomTryOnOrigin; }
            if (state.plugin.empty() && !current->items.empty()) state.plugin = current->items.front().plugin;
            if (state.plugin.empty()) throw std::runtime_error("Draw an outfit first.");
            const auto catalog = ScanTryOnParts(actor, state.plugin, true);
            std::vector<const TryOnPart*> ordered;
            for (const auto& [id, part] : catalog.parts) ordered.push_back(&part);
            std::stable_sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) {
                return RandomTryOn::LowerAscii(a->item.name) < RandomTryOn::LowerAscii(b->item.name);
            });
            for (const auto* part : ordered) {
                state.rows.push_back(part->item);
                state.slots.push_back(part->slots);
            }
            // Retain already-tried pieces even if a subsequent scan omits them.
            for (const auto& item : current->items) {
                if (std::none_of(state.rows.begin(), state.rows.end(), [&](const auto& row) {
                    return ExchangeItemKey(row) == ExchangeItemKey(item);
                })) {
                    auto* form = ResolveItem(item);
                    auto* armor = form ? form->As<RE::TESObjectARMO>() : nullptr;
                    if (!armor) throw std::runtime_error("Equipment changed. Draw again.");
                    state.rows.push_back(item); state.slots.push_back(armor->GetSlotMask().underlying());
                }
            }
            state.selected.resize(state.rows.size());
            for (std::size_t n = 0; n < state.rows.size(); ++n)
                state.selected[n] = std::any_of(current->items.begin(), current->items.end(), [&](const auto& item) {
                    return ExchangeItemKey(item) == ExchangeItemKey(state.rows[n]);
                });
            state.original = state.seen = state.selected;
            state.current = *current;
        } else {
            if (!state.ready || !state.error.empty() || !SameTryOnItems(*current, state.current))
                throw std::runtime_error("Equipment changed. Reopen customization.");
            auto chosen = state.reset ? state.original : RandomTryOn::CustomizeSelection(
                state.slots, state.selected, state.action, state.enabled);
            Preset preset; preset.name = state.current.name;
            for (std::size_t n = 0; n < state.rows.size(); ++n)
                if (chosen[n]) preset.items.push_back(state.rows[n]);
            ApplyTransmog(preset, actor, true);
            auto applied = GetTransmogTryOnPreset(actor);
            if (!applied || !SameTryOnItems(preset, *applied)) {
                if (applied) ApplyTransmog(state.current, actor, true);
                throw std::runtime_error("Equipment could not be applied. Reopen customization.");
            }
            state.selected = std::move(chosen);
            for (std::size_t n = 0; n < state.rows.size(); ++n)
                state.seen[n] = state.seen[n] || state.selected[n];
            state.current = *applied;
            SKSE::log::info("Random try-on customized plugin={} items={}", state.plugin, applied->items.size());
        }
    } catch (const std::exception& e) { state.error = e.what(); }
    state.ready = true;
    { std::scoped_lock lock(randomTryOnMutex); tryOnCustom = std::move(state); }
    tryOnCustomPending = false;
}

void ProcessTryOnInventory(unsigned request, RE::Actor* actor) {
    if (request != kPrepareTryOnItems && request != kAcquireTryOnItems && !tryOnInventoryPending) return;
    TryOnInventoryState state;
    std::string filter; bool exact{};
    { std::scoped_lock lock(randomTryOnMutex);
      state = tryOnInventory; filter = requestedPluginFilter; exact = requestedExactPlugin; }
    try {
        if (!actor || state.session != studioOpenSerial.load())
            throw std::runtime_error("Gallery session changed. Open the list again.");
        if (request == kPrepareTryOnItems) {
            auto* data = RE::TESDataHandler::GetSingleton();
            if (!data) throw std::runtime_error("Game data unavailable.");
            if (!exact) {
                const auto current = GetTransmogTryOnPreset(actor);
                if (current) {
                    std::scoped_lock lock(randomTryOnMutex);
                    if (randomTryOnOriginSession == state.session) {filter=randomTryOnOrigin; exact=true;}
                }
            }
            const auto query = RandomTryOn::LowerAscii(filter);
            if (!exact && query.empty()) throw std::runtime_error("Select one plugin from the dropdown first.");
            std::set<std::string> plugins;
            for (auto* armor : data->GetFormArray<RE::TESObjectARMO>()) {
                if (!armor || armor->IsDeleted() || armor->IsIgnored() ||
                    (armor->GetFormFlags() & RE::TESObjectARMO::RecordFlags::kNonPlayable)) continue;
                const auto* owner = FormOwner(data, armor->GetFormID());
                if (!owner || !armor->GetName() || !*armor->GetName()) continue;
                const auto name = std::string(owner->GetFilename());
                const auto* winner = armor->GetFile();
                const auto match = RandomTryOn::MatchEquipmentPlugin(name,
                    winner ? winner->GetFilename() : owner->GetFilename(), query, exact);
                if (match.empty()) continue;
                if (data->LookupForm(LocalFormID(armor->GetFormID()), owner->GetFilename()) != armor) continue;
                plugins.insert(match);
                state.rows.push_back({name, LocalFormID(armor->GetFormID()), armor->GetName(), "armor", {}, {}, {}});
            }
            if (plugins.size() != 1) throw std::runtime_error("Select one plugin from the dropdown first.");
            std::stable_sort(state.rows.begin(), state.rows.end(), [](const auto& a, const auto& b) {
                return RandomTryOn::LowerAscii(a.name) < RandomTryOn::LowerAscii(b.name);
            });
            state.plugin = *plugins.begin(); state.ready = true;
            SKSE::log::info("Equipment acquisition selection={} exact={} items={}", state.plugin, exact, state.rows.size());
        } else {
            if (request == kAcquireTryOnItems) {
                if (!state.ready || !state.error.empty() || state.obtaining || state.offset)
                    throw std::runtime_error("Open the list again before acquiring items.");
                state.obtaining = true;
            }
            if (state.obtaining) {
                const auto end = std::min(state.offset + 16, state.rows.size());
                while (state.offset < end) {
                    auto* form = ResolveItem(state.rows[state.offset]);
                    auto* armor = form ? form->As<RE::TESObjectARMO>() : nullptr;
                    if (!armor) throw std::runtime_error("Equipment is no longer available.");
                    actor->AddObjectToContainer(armor, nullptr, 1, nullptr);
                    ++state.offset;
                }
                if (state.offset == state.rows.size()) {
                    state.obtaining = false;
                    SetStatus((EquipmentText("装備を各1個取得しました：", "Acquired one of each item: ")) + std::to_string(state.offset));
                    SKSE::log::info("Random try-on inventory acquired plugin={} count={}", state.plugin, state.offset);
                }
            }
        }
    } catch (const std::exception& e) { state.ready = true; state.obtaining = false; state.error = e.what(); }
    const bool pending = state.obtaining;
    { std::scoped_lock lock(randomTryOnMutex); tryOnInventory = std::move(state); }
    tryOnInventoryPending = pending;
}

std::string TryOnSlotLabel(std::uint32_t mask) {
    std::string result;
    for (unsigned bit = 0; bit < 32; ++bit) if (mask & (1u << bit)) {
        if (!result.empty()) result += ',';
        result += std::to_string(bit + 30);
    }
    return result;
}

void RenderTryOnDialogs() {
    const auto display = UI::GetIO()->DisplaySize;
    const auto framebuffer = UI::GetIO()->DisplayFramebufferScale;
    const auto layout = ResolveLayoutSize({display.x,display.y},{framebuffer.x,framebuffer.y},ReadLayoutSurface());
    if (tryOnCustomVisible) {
      UI::SetNextWindowPos({layout.width*.01f,layout.height*.10f},UI::ImGuiCond_Always);
      UI::SetNextWindowSize({layout.width*.515f,layout.height*.80f},UI::ImGuiCond_Always);
      if (UI::Begin(EquipmentText("装備カスタマイズ###tryOnCustom", "Customize equipment###tryOnCustom"),
                    &tryOnCustomVisible, UI::ImGuiWindowFlags_NoSavedSettings | UI::ImGuiWindowFlags_NoCollapse)) {
        TryOnCustomState state; { std::scoped_lock lock(randomTryOnMutex); state = tryOnCustom; }
        UI::TextWrapped("%s", EquipmentText("チェックを外すと解除、戻すと装着。同じスロットは交換します。", "Uncheck to remove; check to add. Conflicting slots are replaced."));
        UI::TextWrapped("%s", EquipmentText("衣服を外すと肌が露出する場合があります。", "Removing clothing can expose the body."));
        UI::TextWrapped("%s", state.plugin.c_str());
        UI::TextWrapped("%s", EquipmentText("右一覧：[装着中]＝現在使用、[左一覧]＝追加済み・解除中", "Right list: [Worn] = active; [Listed] = added to left list, currently removed."));
        if (!state.error.empty()) UI::TextWrapped("%s", state.error.c_str());
        if (!state.ready) UI::TextUnformatted(EquipmentText("取得中…", "Loading..."));
        static char search[128]{};
        UI::InputText(EquipmentText("装備を検索###customSearch", "Search###customSearch"), search, sizeof(search));
        UI::BeginDisabled(tryOnCustomPending || !state.ready || !state.error.empty() || saveBusy || verificationTicks);
        if (UI::BeginTable("TryOnParts", 2)) {
            UI::TableNextColumn(); UI::TextUnformatted(EquipmentText("現在の試着装備", "Current try-on"));
            UI::BeginChild("TryOnCurrent", {0, -UI::GetFrameHeightWithSpacing()*2});
            for (std::size_t n = 0; n < state.rows.size(); ++n) if (state.seen[n]) {
                bool chosen = state.selected[n]; UI::PushID(static_cast<int>(n));
                const auto label = "[" + TryOnSlotLabel(state.slots[n]) + "] " + state.rows[n].name;
                if (UI::Checkbox(label.c_str(), &chosen)) {
                    { std::scoped_lock lock(randomTryOnMutex); tryOnCustom.action=n; tryOnCustom.enabled=chosen; tryOnCustom.reset=false; }
                    tryOnCustomPending=true; command=kChangeTryOnCustom;
                }
                UI::PopID();
            }
            UI::EndChild(); UI::TableNextColumn();
            UI::TextUnformatted(EquipmentText("プラグイン内の試着可能な装備", "Compatible plugin equipment"));
            UI::BeginChild("TryOnAvailable", {0, -UI::GetFrameHeightWithSpacing()*2});
            const auto query = RandomTryOn::LowerAscii(search);
            for (std::size_t n = 0; n < state.rows.size(); ++n) {
                if (RandomTryOn::LowerAscii(state.rows[n].name).find(query) == std::string::npos) continue;
                UI::PushID(static_cast<int>(n));
                const auto mark = state.selected[n] ? (EquipmentText("[装着中] ", "[Worn] ")) :
                    state.seen[n] ? (EquipmentText("[左一覧] ", "[Listed] ")) : "";
                const auto label = std::string(mark) + "[" + TryOnSlotLabel(state.slots[n]) + "] " + state.rows[n].name;
                if (UI::Selectable(label.c_str(), state.selected[n])) {
                    { std::scoped_lock lock(randomTryOnMutex); tryOnCustom.action=n; tryOnCustom.enabled=true; tryOnCustom.reset=false; }
                    tryOnCustomPending=true; command=kChangeTryOnCustom;
                }
                UI::PopID();
            }
            UI::EndChild(); UI::EndTable();
        }
        if (UI::Button(EquipmentText("開いた時の組み合わせに戻す", "Reset to opening outfit"))) {
            { std::scoped_lock lock(randomTryOnMutex); tryOnCustom.reset=true; }
            tryOnCustomPending=true; command=kChangeTryOnCustom;
        }
        UI::EndDisabled(); UI::SameLine();
        UI::BeginDisabled(tryOnCustomPending);
        if (UI::Button(EquipmentText("閉じる", "Close"))) tryOnCustomVisible=false;
        UI::EndDisabled();
      }
      UI::End();
    }
    UI::SetNextWindowSize({layout.width*.48f,layout.height*.65f}, UI::ImGuiCond_Appearing);
    if (UI::BeginPopupModal("装備を一括取得 / Acquire###tryOnInventory", nullptr, UI::ImGuiWindowFlags_NoSavedSettings)) {
        TryOnInventoryState state; { std::scoped_lock lock(randomTryOnMutex); state=tryOnInventory; }
        UI::TextWrapped("%s",state.plugin.c_str());
        UI::TextWrapped(EquipmentText("対象 %zu種類を各1個取得します。色違い・差分も含み、所持済みでも追加します。", "Acquire one of each of %zu items, including variants and items already owned."), state.rows.size());
        if (!state.error.empty()) UI::TextWrapped("%s",state.error.c_str());
        if (!state.ready) UI::TextUnformatted(EquipmentText("取得中…", "Loading..."));
        UI::BeginChild("AcquisitionList", {0,-UI::GetFrameHeightWithSpacing()*3});
        for (const auto& item : state.rows) UI::TextUnformatted(item.name.c_str());
        UI::EndChild();
        UI::Text("%zu / %zu",state.offset,state.rows.size());
        UI::BeginDisabled(tryOnInventoryPending || !state.ready || !state.error.empty() || state.offset || state.rows.empty());
        if (UI::Button(EquipmentText("各1個をインベントリへ追加", "Add one of each to inventory"))) {
            tryOnInventoryPending=true; command=kAcquireTryOnItems;
        }
        UI::EndDisabled(); UI::SameLine();
        UI::BeginDisabled(tryOnInventoryPending);
        if (UI::Button(EquipmentText("閉じる", "Close"))) UI::CloseCurrentPopup();
        UI::EndDisabled(); UI::EndPopup();
    }
}
