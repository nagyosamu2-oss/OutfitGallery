#pragma once
namespace Gallery::SurvivalRegion {
// Native CTDA function IDs: GetPos, GetGlobalValue, IsPlayerInRegion.
// Includes the read-only global query observed at runtime; no mutation calls.
constexpr bool ReadOnlyCondition(unsigned function) {return function==6 || function==74 || function==117;}

// Matches the official Survival_PlayerLocationInfo.GetCurrentAreaType order.
// Unknown regions default to Cool, never Freezing. Weather/time are excluded.
constexpr bool IsFreezing(bool warm,bool cool,bool freezing,bool southMountains,bool fallMountains) {
    return southMountains || fallMountains || (!warm && !cool && freezing);
}
constexpr int Area(bool warm,bool cool,bool freezing,bool southMountains,bool fallMountains) {
    return southMountains || fallMountains?3:warm?1:cool?2:freezing?3:2;
}
enum class Weather {Clear,Rain,Snow,Blizzard};
// Official environmental cold scale; excludes accumulated cold and HUD/heat.
constexpr int ColdLevel(int area,float hour,Weather weather) {
    if(area<0 || area>3 || !(hour>=0.f && hour<24.f))return -1;
    if(area==0)return 0;
    int value=area==1?0:area==2?3:6;
    if(hour<7.f || hour>19.f)value+=area==1?1:area==2?2:4;
    return value+(weather==Weather::Blizzard?10:weather==Weather::Snow?6:weather==Weather::Rain?3:0);
}
}
