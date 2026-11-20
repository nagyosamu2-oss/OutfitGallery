struct SurvivalColdRegionState {bool ready{};int area{},level{},stage{1};};
// Evaluate the official read-only region/position queries and runtime-added
// global reads observed in the player's process. Never run mutation functions.
// Unknown patched functions decline classification rather than execute them.
bool SafeSurvivalRegionConditions(const RE::TESCondition& conditions) {
    if(!conditions.head)return false;
    unsigned count=0;
    for(const auto* item=conditions.head;item;item=item->next) {
        if(++count>128)return false;
        const auto& data=item->data;
        const auto function=data.functionData.function.underlying();
        if(!SurvivalRegion::ReadOnlyCondition(function))return false;
        if(data.flags.usesAliases || data.flags.usePackData || data.flags.global || data.object.underlying()!=static_cast<std::uint8_t>(RE::CONDITIONITEMOBJECT::kSelf))return false;
    }
    return true;
}
// Game thread, read only. Evaluate official region-effect conditions without
// applying the spell or starting Survival Mode, quests, scripts or penalties.
SurvivalColdRegionState ReadSurvivalColdRegion(RE::Actor* player,float hour) {
    SurvivalColdRegionState result;
    auto* data=RE::TESDataHandler::GetSingleton();
    if(!data || !player || player!=RE::PlayerCharacter::GetSingleton() || !(hour>=0.f && hour<24.f))return result;
    constexpr auto plugin="ccQDRSSE001-SurvivalMode.esl";
    result.stage=2;
    const auto* interiors=data->LookupForm<RE::BGSListForm>(0x92F,plugin);
    if(!interiors)return result;
    result.stage=3;
    const auto* cell=player->GetParentCell();
    if(!cell)return result;
    if(cell->IsInteriorCell() || interiors->HasForm(player->GetWorldspace())) {result.stage=4;result.ready=true;return result;}
    result.stage=5;
    const auto* spell=data->LookupForm<RE::SpellItem>(0x87F,plugin);
    if(!spell)return result;
    // Match by effect form, not order. Patches can reorder the five effects.
    constexpr std::array<RE::FormID,5> ids{0x83C,0x83D,0x83E,0x840,0x86A};
    result.stage=6;
    std::array<bool,5> found{},matches{};
    for(const auto* effect:spell->effects) {
        if(!effect || !effect->baseEffect)continue;
        for(std::size_t i=0;i<ids.size();++i) {
            if(effect->baseEffect!=data->LookupForm<RE::EffectSetting>(ids[i],plugin))continue;
            if(found[i] || effect->baseEffect->conditions.head || !SafeSurvivalRegionConditions(effect->conditions))return result;
            found[i]=true;matches[i]=effect->conditions.IsTrue(player,player);
        }
    }
    result.stage=7;
    if(!std::all_of(found.begin(),found.end(),[](bool value){return value;}))return result;
    result.stage=8;
    const auto* sky=RE::Sky::GetSingleton();if(!sky)return result;
    result.stage=9;
    const auto* weather=sky->currentWeatherPct>=.5f?sky->currentWeather:sky->lastWeather;
    if(!weather)return result;
    result.stage=10;
    const auto* ash=data->LookupForm<RE::BGSListForm>(0x8A8,plugin);
    const auto* blizzard=data->LookupForm<RE::BGSListForm>(0x8A9,plugin);
    if(!ash || !blizzard)return result;
    auto kind=SurvivalRegion::Weather::Clear;
    if(weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kSnow)) {
        if(!ash->HasForm(weather))kind=blizzard->HasForm(weather)?SurvivalRegion::Weather::Blizzard:SurvivalRegion::Weather::Snow;
    } else if(weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy))kind=SurvivalRegion::Weather::Rain;
    result.stage=11;
    result.area=SurvivalRegion::Area(matches[0],matches[1],matches[2],matches[3],matches[4]);
    result.level=SurvivalRegion::ColdLevel(result.area,hour,kind);result.ready=result.level>=0;
    return result;
}
