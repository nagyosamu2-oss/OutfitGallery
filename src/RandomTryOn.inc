constexpr unsigned kRandomTryOn = 36;
constexpr unsigned kEndRandomTryOn = 37;
constexpr unsigned kRerollTryOn = 42;
std::mutex randomTryOnMutex;
std::string requestedPluginFilter;
std::string randomTryOnOrigin;
unsigned randomTryOnOriginSession{};
bool requestedFillSlots{};
bool requestedExactPlugin{};
std::atomic<bool> randomPluginListRequested{};
unsigned randomPluginListSerial{};
struct TryOnPluginChoice { std::string name, search; bool tryOnBody{}; std::size_t count{}; };
std::shared_ptr<const std::vector<TryOnPluginChoice>> randomPluginList;

namespace {
using Armor = RE::TESObjectARMO;
using Slot = RE::BIPED_MODEL::BipedObjectSlot;
constexpr auto bodySlot = static_cast<std::uint32_t>(Slot::kBody);
constexpr auto shieldSlot = static_cast<std::uint32_t>(Slot::kShield);

struct TryOnPart {
    Item item;
    std::uint32_t slots{};
    bool torso{};
    RandomTryOn::OutfitName outfitName;
    std::string winningPlugin;
};
struct TryOnCandidate {
    RE::FormID id{};
    std::vector<const TryOnPart*> parts;
};
struct TryOnPlugin {
    std::map<std::string, std::vector<const TryOnPart*>> families;
    std::vector<TryOnCandidate> explicitOutfits;
    std::vector<TryOnCandidate> inferredOutfits;
};

std::uint32_t TryOnSlots(Armor* armor, RE::TESRace* race, bool female) {
    const auto slots = armor->GetSlotMask().underlying();
    if (!slots || (slots & shieldSlot)) return 0;
    bool hasModel = false;
    for (auto* addon : armor->armorAddons) {
        if (!addon || !addon->IsValidRace(race)) continue;
        const auto* male = addon->bipedModels[0].GetModel();
        const auto* femaleModel = addon->bipedModels[1].GetModel();
        if (!HasAppearanceModel(female, male && *male, femaleModel && *femaleModel)) continue;
        if (addon->GetSlotMask().underlying() & shieldSlot) return 0;
        hasModel = true;
    }
    // Equip conflicts use the ARMO slots. An ARMA can include extra
    // body partitions that a separate wearable (e.g. calf) covers.
    return hasModel ? slots : 0;
}

void FillTryOnSlots(Preset& preset, const TryOnPlugin& plugin, const TryOnCandidate& selected,
                    const TryOnPart& body, std::mt19937& rng, bool commonNames = false) {
    using namespace RandomTryOn;
    const auto torso = ParseFillName(body.outfitName);
    auto color = torso.color;
    std::string_view series;
    if (commonNames) {
        for (const auto& [family, members] : plugin.families) {
            for (const auto* part : members) {
                if (part->torso || (part->slots & body.slots)) continue;
                const auto prefix = CommonSeriesPrefix(torso.series, ParseFillName(part->outfitName).series);
                if (prefix.size() > series.size()) series = prefix;
            }
        }
    }
    std::uint32_t usedSlots{};
    std::vector<std::string_view> usedRoles;
    for (const auto* part : selected.parts) {
        usedSlots |= part->slots;
        usedRoles.push_back(FillRole(part->outfitName.part));
        if (color.empty()) color = ParseFillName(part->outfitName).color;
    }

    struct Candidate {
        const TryOnPart* part;
        FillName name;
        VariantMatch variant;
        std::string_view role;
    };
    std::vector<Candidate> candidates;
    std::vector<std::string_view> colors;
    for (const auto& [family, members] : plugin.families) {
        for (const auto* part : members) {
            if (part->torso || (part->slots & usedSlots)) continue;
            const auto variant = MatchVariant(body.outfitName.variant, part->outfitName.variant);
            if (!commonNames && variant == VariantMatch::Incompatible) continue;
            const auto name = ParseFillName(part->outfitName);
            if (commonNames && !HasSeriesPrefix(name.series, series)) continue;
            candidates.push_back({part, name, variant, FillRole(part->outfitName.part)});
            if (color.empty() && !name.color.empty() && std::find(colors.begin(), colors.end(), name.color) == colors.end())
                colors.push_back(name.color);
        }
    }
    if (color.empty() && !colors.empty())
        color = colors[std::uniform_int_distribution<std::size_t>(0, colors.size() - 1)(rng)];
    std::erase_if(candidates, [&](const auto& candidate) { return FillMatch(torso, candidate.name, color) < 0; });
    std::shuffle(candidates.begin(), candidates.end(), rng);
    const auto priority = [&](const auto& candidate) {
        const auto role = candidate.role;
        const auto order = role == "lower" ? 0 : role == "underwear" ? 2 : 1;
        return std::tuple{order, NumberMatch(body.outfitName.number, candidate.part->outfitName.number),
            FillMatch(torso, candidate.name, color),
            commonNames ? VariantMatch::Exact : candidate.variant};
    };
    std::stable_sort(candidates.begin(), candidates.end(),
        [&](const auto& a, const auto& b) { return priority(a) < priority(b); });

    for (const auto& candidate : candidates) {
        const auto* part = candidate.part;
        if ((usedSlots & part->slots) || (!commonNames && !candidate.role.empty() &&
            std::find(usedRoles.begin(), usedRoles.end(), candidate.role) != usedRoles.end()))
            continue;
        usedSlots |= part->slots;
        usedRoles.push_back(candidate.role);
        preset.items.push_back(part->item);
    }
}


struct TryOnCatalog {
    std::map<std::string, TryOnPlugin> plugins;
    std::map<RE::FormID, TryOnPart> parts;
};

TryOnCatalog ScanTryOnParts(RE::Actor* actor, std::string_view filter, bool exactPlugin) {
    auto* data = RE::TESDataHandler::GetSingleton();
    auto* race = actor->GetRace();
    const auto* base = actor->GetActorBase();
    if (!data || !race || !base) return {};
    static constexpr std::array baseFiles{
        "skyrim.esm"sv, "update.esm"sv, "dawnguard.esm"sv, "hearthfires.esm"sv, "dragonborn.esm"sv
    };
    const auto query = RandomTryOn::LowerAscii(filter);
    TryOnCatalog catalog;
    for (auto* armor : data->GetFormArray<Armor>()) {
        if (!armor || armor->IsDeleted() || armor->IsIgnored() ||
            (armor->GetFormFlags() & Armor::RecordFlags::kNonPlayable)) continue;
        const auto* name = armor->GetName();
        if (!name || !*name) continue;
        const auto id = armor->GetFormID();
        const auto* file = FormOwner(data, id);
        if (!file) continue;
        const auto filename = file->GetFilename();
        const auto* winner = armor->GetFile();
        const auto winningName = winner ? winner->GetFilename() : filename;
        const auto lower = RandomTryOn::LowerAscii(filename);
        const bool matches = !RandomTryOn::MatchEquipmentPlugin(filename, winningName, query, exactPlugin).empty();
        if (!matches || std::find(baseFiles.begin(), baseFiles.end(), lower) != baseFiles.end()) continue;
        const auto slots = TryOnSlots(armor, race, base->IsFemale());
        if (!slots || data->LookupForm(LocalFormID(id), filename) != armor) continue;
        const auto [part, added] = catalog.parts.try_emplace(id, TryOnPart{
            {std::string(filename), LocalFormID(id), name, "armor", {}, {}, {}}, slots,
            (armor->GetSlotMask().underlying() & bodySlot) != 0, RandomTryOn::ParseOutfitName(name),
            std::string(winningName)
        });
        if (added)
            catalog.plugins[std::string(filename)].families[part->second.outfitName.family].push_back(&part->second);
    }
    // Conversion patches can introduce new pieces alongside their overrides.
    // Keep each item's real identity, but make compatible, same-series additions
    // available to the original body's plugin pool too.
    std::map<std::string, std::set<std::string>> patchTargets;
    for (const auto& [id, part] : catalog.parts)
        if (part.torso && part.winningPlugin != part.item.plugin)
            patchTargets[part.winningPlugin].insert(part.item.plugin);
    if (!patchTargets.empty()) for (auto* armor : data->GetFormArray<Armor>()) {
        if (!armor || armor->IsDeleted() || armor->IsIgnored() ||
            (armor->GetFormFlags() & Armor::RecordFlags::kNonPlayable) ||
            (armor->GetSlotMask().underlying() & bodySlot)) continue;
        const auto id = armor->GetFormID();
        const auto* owner = FormOwner(data, id);
        const auto* name = armor->GetName();
        if (!owner || !name || !*name) continue;
        const auto targets = patchTargets.find(std::string(owner->GetFilename()));
        if (targets == patchTargets.end()) continue;
        const auto slots = TryOnSlots(armor, race, base->IsFemale());
        if (!slots || data->LookupForm(LocalFormID(id), owner->GetFilename()) != armor) continue;
        const auto parsed = RandomTryOn::ParseOutfitName(name);
        for (const auto& original : targets->second) {
            auto& plugin = catalog.plugins.at(original);
            bool related = false;
            for (const auto& [family, members] : plugin.families) {
                for (const auto* body : members) if (body->torso &&
                    !RandomTryOn::CommonSeriesPrefix(RandomTryOn::ParseFillName(body->outfitName).series,
                        RandomTryOn::ParseFillName(parsed).series).empty()) {related=true; break;}
                if (related) break;
            }
            if (!related) continue;
            const auto* winner = armor->GetFile();
            const auto [part, added] = catalog.parts.try_emplace(id, TryOnPart{
                {std::string(owner->GetFilename()), LocalFormID(id), name, "armor", {}, {}, {}},
                slots, false, parsed, std::string(winner ? winner->GetFilename() : owner->GetFilename())});
            auto& members = plugin.families[part->second.outfitName.family];
            if (std::find(members.begin(), members.end(), &part->second) == members.end())
                members.push_back(&part->second);
        }
    }
    return catalog;
}

std::pair<std::string, Preset> DrawRandomOutfit(RE::Actor* actor, std::string_view filter, bool fillSlots,
                                               bool exactPlugin, RE::FormID displayedBody = 0) {
    auto* data = RE::TESDataHandler::GetSingleton();
    if (!data) return {};
    auto [plugins, parts] = ScanTryOnParts(actor, filter, exactPlugin);
    for (auto* outfit : data->GetFormArray<RE::BGSOutfit>()) {
        if (!outfit || outfit->IsDeleted() || outfit->IsIgnored()) continue;
        const auto* file = FormOwner(data, outfit->GetFormID());
        if (!file) continue;
        const auto plugin = plugins.find(std::string(file->GetFilename()));
        if (plugin == plugins.end()) continue;
        TryOnCandidate candidate{outfit->GetFormID(), {}};
        std::uint32_t slots{};
        bool complete = true;
        bool hasTorso = false;
        for (auto* item : outfit->outfitItems) {
            const auto part = item ? parts.find(item->GetFormID()) : parts.end();
            if (part == parts.end() || part->second.item.plugin != plugin->first ||
                (slots & part->second.slots)) {
                complete = false;
                break;
            }
            slots |= part->second.slots;
            hasTorso |= part->second.torso;
            candidate.parts.push_back(&part->second);
        }
        if (complete && hasTorso) plugin->second.explicitOutfits.push_back(std::move(candidate));
    }

    static std::mt19937 rng(std::random_device{}());
    static RE::FormID lastDraw{};
    const auto avoid = displayedBody ? displayedBody : lastDraw;
    struct Pool {
        std::vector<TryOnCandidate>* candidates;
        TryOnPlugin* plugin;
    };
    std::vector<Pool> pools;
    bool hasAlternative = false;
    for (auto& [name, plugin] : plugins) {
        auto& candidates = plugin.explicitOutfits.empty() ? plugin.inferredOutfits : plugin.explicitOutfits;
        if (plugin.explicitOutfits.empty()) {
            for (const auto& [family, members] : plugin.families) {
                for (auto* body : members) {
                    if (!body->torso) continue;
                    candidates.push_back({data->LookupFormID(body->item.localID, body->item.plugin), {body}});
                }
            }
        }
        if (candidates.empty()) continue;
        pools.push_back({&candidates, &plugin});
        hasAlternative |= std::any_of(candidates.begin(), candidates.end(),
            [avoid](const auto& candidate) { return candidate.id != avoid; });
    }
    if (hasAlternative) {
        for (auto& pool : pools)
            std::erase_if(*pool.candidates, [avoid](const auto& candidate) { return candidate.id == avoid; });
        std::erase_if(pools, [](const auto& pool) { return pool.candidates->empty(); });
    }
    if (pools.empty()) return {};
    const auto& pool = pools[std::uniform_int_distribution<std::size_t>(0, pools.size() - 1)(rng)];
    const auto& candidates = *pool.candidates;
    const auto& selected = candidates[std::uniform_int_distribution<std::size_t>(0, candidates.size() - 1)(rng)];
    Preset preset;
    for (const auto* part : selected.parts) preset.items.push_back(part->item);
    const auto torso = std::find_if(selected.parts.begin(), selected.parts.end(),
        [](const auto* part) { return part->torso; });
    if (fillSlots) {
        FillTryOnSlots(preset, *pool.plugin, selected, **torso, rng);
    } else {
        FillTryOnSlots(preset, *pool.plugin, selected, **torso, rng, true);
    }
    preset.name = preset.items.front().name;
    lastDraw = selected.id;
    return {(*torso)->winningPlugin, std::move(preset)};
}
}

std::uint64_t TryOnPluginCreationTime(std::string_view filename) {
    const auto path = std::filesystem::path("Data") / filename;
    const HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 0;
    FILETIME created{};
    const bool found = GetFileTime(file, &created, nullptr, nullptr) != 0;
    CloseHandle(file);
    return found ? (static_cast<std::uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime : 0;
}

void RefreshRandomTryOnPlugins(RE::Actor* actor) {
    const auto started = GetTickCount64();
    auto list = std::make_shared<std::vector<TryOnPluginChoice>>();
    try {
        const auto catalog = ScanTryOnParts(actor, {}, false);
        std::map<std::string, std::uint64_t> dates;
        std::map<std::string, std::uint64_t> fileDates;
        RandomTryOn::EquipmentPluginCatalog equipment;
        const auto creationTime = [&](const std::string& name) {
            auto [entry, added] = fileDates.try_emplace(name);
            if (added) entry->second = TryOnPluginCreationTime(name);
            return entry->second;
        };
        auto* data = RE::TESDataHandler::GetSingleton();
        if (!data) throw std::runtime_error("Game data unavailable.");
        for (auto* armor : data->GetFormArray<Armor>()) {
            if (!armor || armor->IsDeleted() || armor->IsIgnored() ||
                (armor->GetFormFlags() & Armor::RecordFlags::kNonPlayable) ||
                !armor->GetName() || !*armor->GetName()) continue;
            const auto id = armor->GetFormID();
            const auto* owner = FormOwner(data, id);
            if (!owner || data->LookupForm(LocalFormID(id), owner->GetFilename()) != armor) continue;
            const auto* winner = armor->GetFile();
            const auto part = catalog.parts.find(id);
            const bool body = part != catalog.parts.end() && part->second.torso;
            RandomTryOn::AddEquipmentPlugin(equipment, owner->GetFilename(),
                winner ? winner->GetFilename() : owner->GetFilename(), body, id);
        }
        const auto beforeCollapse = equipment.size();
        const auto collapseStarted = GetTickCount64();
        equipment = RandomTryOn::CollapseEquipmentPlugins(std::move(equipment));
        const auto collapseMs = GetTickCount64() - collapseStarted;
        for (const auto& [name, entry] : equipment) {
            std::string search = RandomTryOn::LowerAscii(name);
            auto date = creationTime(name);
            for (const auto& alias : entry.aliases) {
                search += '\n' + RandomTryOn::LowerAscii(alias);
                date = std::max(date, creationTime(alias));
            }
            dates[name] = date;
            list->push_back({name, std::move(search), entry.tryOnBody, entry.count});
        }
        std::stable_sort(list->begin(), list->end(), [&](const auto& a, const auto& b) {
            const auto left = dates.at(a.name), right = dates.at(b.name);
            return left != right ? left > right : a.name < b.name;
        });
        SKSE::log::info("Equipment plugin list session={} before={} shown={} collapsed={} dedupMs={} totalMs={}",
            studioOpenSerial.load(), beforeCollapse, list->size(), beforeCollapse-list->size(),
            collapseMs, GetTickCount64()-started);
    } catch (const std::exception& e) {
        SKSE::log::warn("Random try-on plugin list failed: {}", e.what());
        list->clear();
    }
    std::scoped_lock lock(randomTryOnMutex);
    randomPluginList = std::move(list);
    randomPluginListSerial = studioOpenSerial.load();
    randomPluginListRequested = false;
}

void RunRandomTryOn(RE::Actor* actor, bool reroll = false) {
    try {
        std::string filter;
        RE::FormID displayedBody{};
        bool fillSlots, exactPlugin;
        {
            std::scoped_lock lock(randomTryOnMutex);
            filter = requestedPluginFilter;
            fillSlots = requestedFillSlots;
            exactPlugin = requestedExactPlugin;
        }
        if (reroll) {
            if (!actor) throw std::runtime_error("Target unavailable.");
            const auto appearanceBody = AppearanceBodyArmorID(actor);
            if (appearanceBody) displayedBody = *appearanceBody;
            else for (const auto& item : SnapshotRemovalEquipment(actor))
                if (item.slots & bodySlot) {displayedBody=item.id; break;}
            auto* data = RE::TESDataHandler::GetSingleton();
            const auto* owner = data && displayedBody ? FormOwner(data, displayedBody) : nullptr;
            if (!owner) {
                SetStatus(EquipmentText("表示中のボディ装備がないため、再抽選できません。", "No displayed body equipment to reroll."));
                return;
            }
            filter=owner->GetFilename(); exactPlugin=true;
            SKSE::log::info("Random try-on reroll displayedBody={:08X} appearanceControlled={} originalPlugin={}",
                displayedBody, appearanceBody.has_value(), filter);
        }
        auto [plugin, preset] = DrawRandomOutfit(actor, filter, fillSlots, exactPlugin, displayedBody);
        if (preset.items.empty()) {
            SetStatus(EquipmentText("試着できる装備が見つかりませんでした。", "No outfits matched for try-on."));
            return;
        }
        ApplyTransmog(preset, actor, true);
        const auto applied = GetTransmogTryOnPreset(actor);
        if (!applied) throw std::runtime_error("The try-on appearance could not be retained.");
        { std::scoped_lock lock(randomTryOnMutex);
          randomTryOnOrigin = preset.items.front().plugin; randomTryOnOriginSession = studioOpenSerial.load(); }
        std::string names;
        for (const auto& item : applied->items) {
            if (!names.empty()) names += " / ";
            names += item.name;
        }
        SetStatus((EquipmentText("試着中  ", "Trying on  ")) + plugin);
        SKSE::log::info("Random try-on plugin={} mode={} requested={} accepted={} items={}",
            plugin, fillSlots ? "fill" : "common-name", preset.items.size(), applied->items.size(), names);
    } catch (const std::exception& e) {
        SKSE::log::warn("Random try-on failed: {}", e.what());
        SetStatus(language == 1 ? "試着できませんでした。詳細は OutfitGallery.log を確認してください。" : e.what());
    }
}

#include "TryOnCustomization.inc"

void RenderRandomTryOn() {
    { std::scoped_lock lock(randomTryOnMutex);
      if (tryOnCustom.session != studioOpenSerial.load()) {tryOnCustomPending=false; tryOnCustomVisible=false;} }
    if (!UI::CollapsingHeader(EquipmentText("装備取得・おまかせ試着###randomTryOn", "Equipment & random try-on###randomTryOn")))
        return;
    static char filter[128]{};
    static std::string selectedPlugin;
    UI::SetNextItemWidth(UI::GetContentRegionAvail().x * 0.55f);
    if (UI::InputText(EquipmentText("プラグイン名###tryOnFilter", "Plugin name###tryOnFilter"),
                      filter, sizeof(filter)))
        selectedPlugin.clear();
    UI::SetNextItemWidth(UI::GetContentRegionAvail().x * 0.55f);
    if (UI::BeginCombo(EquipmentText("装備プラグイン（新しい順）###tryOnPluginList", "Equipment plugins (newest first)###tryOnPluginList"),
                       selectedPlugin.empty() ? (EquipmentText("入力欄で絞り込み", "Use text filter")) : selectedPlugin.c_str())) {
        if (UI::Selectable(EquipmentText("入力欄で絞り込み", "Use text filter"), selectedPlugin.empty()))
            selectedPlugin.clear();
        std::shared_ptr<const std::vector<TryOnPluginChoice>> list;
        {
            std::scoped_lock lock(randomTryOnMutex);
            if (randomPluginListSerial == studioOpenSerial.load()) list = randomPluginList;
            else randomPluginListRequested = true;
        }
        if (!list) UI::TextUnformatted(EquipmentText("取得中…", "Loading…"));
        else if (list->empty()) UI::TextUnformatted(EquipmentText("取得できる防具がありません", "No eligible armor"));
        else {
            const auto query = RandomTryOn::LowerAscii(filter);
            bool matched = false;
            for (const auto& entry : *list) {
                if (entry.search.find(query) == std::string::npos) continue;
                matched = true;
                const auto label = entry.name + (entry.tryOnBody ? "" :
                    EquipmentText(" ［取得のみ］", " [Acquire only]"));
                if (UI::Selectable(label.c_str(), selectedPlugin == entry.name)) selectedPlugin = entry.name;
                if (UI::IsItemHovered()) {
                    UI::SetTooltip("%s\n%zu", entry.tryOnBody ?
                        EquipmentText("試着・取得に対応した防具があります。", "Armor available for try-on and acquisition.") :
                        EquipmentText("取得できます。抽選用の対応ボディ装備はありません。", "Acquisition available; no compatible body armor for random try-on."), entry.count);
                }
            }
            if (!matched) UI::TextUnformatted(EquipmentText("一致するプラグインがありません", "No matching plugins"));
        }
        UI::EndCombo();
    }
    static bool fillSlots{};
    UI::BeginDisabled(saveBusy || verificationTicks || tryOnCustomPending || tryOnInventoryPending || !TransmogAvailable());
    if (UI::Button(EquipmentText("抽選して試着", "Draw and try on"))) {
        {
            std::scoped_lock lock(randomTryOnMutex);
            requestedExactPlugin = !selectedPlugin.empty();
            requestedPluginFilter = requestedExactPlugin ? selectedPlugin : filter;
            requestedFillSlots = fillSlots;
        }
        command = kRandomTryOn;
        tryOnCustomVisible=false;
    }
    UI::SameLine();
    if (UI::Button(EquipmentText("再抽選", "Reroll"))) {
        { std::scoped_lock lock(randomTryOnMutex); requestedFillSlots=fillSlots; }
        tryOnCustomVisible=false;
        command=kRerollTryOn;
    }
    if (UI::IsItemHovered()) UI::SetTooltip("%s",EquipmentText("現在表示中のボディ装備のプラグイン内で選び直します。保存したセットにも使えます。", "Reroll from the displayed body equipment's plugin, including saved outfits."));
    UI::EndDisabled();
    UI::SameLine();
    UI::Checkbox(EquipmentText("空きスロットを埋める", "Fill empty slots"), &fillSlots);
    UI::BeginDisabled(saveBusy || !TransmogTryOnActive(studioActorID.load()));
    if (UI::Button(EquipmentText("試着前に戻す", "End try-on"))) command = kEndRandomTryOn;
    UI::SameLine();
    if (UI::Button(EquipmentText("試着セットを登録", "Save try-on preset")) && !saveBusy.exchange(true)) {
        { std::scoped_lock lock(presetsMutex); requestedName.clear(); }
        command = 3;
    }
    UI::SameLine();
    if (UI::Button(EquipmentText("装備カスタマイズ", "Customize equipment"))) {
        { std::scoped_lock lock(randomTryOnMutex); tryOnCustom={}; tryOnCustom.session=studioOpenSerial.load(); }
        tryOnCustomPending=true; command=kOpenTryOnCustom;
        tryOnCustomVisible=true;
    }
    UI::EndDisabled();
    UI::SameLine();
    UI::BeginDisabled(saveBusy || verificationTicks || tryOnInventoryPending);
    if (UI::Button(EquipmentText("プラグインの装備を取得", "Acquire plugin equipment..."))) {
        { std::scoped_lock lock(randomTryOnMutex);
          tryOnInventory={}; tryOnInventory.session=studioOpenSerial.load();
          requestedExactPlugin=!selectedPlugin.empty(); requestedPluginFilter=requestedExactPlugin?selectedPlugin:filter; }
        tryOnInventoryPending=true; command=kPrepareTryOnItems;
        UI::OpenPopup("装備を一括取得 / Acquire###tryOnInventory");
    }
    UI::EndDisabled();
    RenderTryOnDialogs();
}
