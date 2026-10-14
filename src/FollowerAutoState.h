#pragma once
#include "Presets.h"
#include <nlohmann/json.hpp>
namespace Gallery::FollowerAuto {
constexpr unsigned Limit=2;
constexpr unsigned Record=0x46415554; // FAUT, separate from FOUT
// Transient arrival checks, never persisted. At most two reads and one repair
// per cell/load transition, plus one early same-outfit check/repair.
struct ArrivalCheck {
    std::uint32_t cell{};
    bool loaded{}, loading{}, pending{}, earlyPending{};
    unsigned checks{};
    std::uint64_t readySince{};
    void Observe(std::uint32_t currentCell,bool isLoaded,bool loadingMenu) {
        if(isLoaded && (!loaded || cell!=currentCell || (loading && !loadingMenu))) {
            earlyPending=cell!=0;
            pending=true;checks=0;readySince=0;
        }
        cell=currentCell;loaded=isLoaded;loading=loadingMenu;
        if(!isLoaded || loadingMenu) readySince=0;
    }
    void Block() {readySince=0;}
    void Cancel() {pending=false;earlyPending=false;readySince=0;}
    bool EarlyReady(bool handled,int stable,int place,const std::string& lastPhoto,const std::string& photo) const {
        return pending && earlyPending && handled && stable==place && !photo.empty() && lastPhoto==photo;
    }
    void EarlyChecked() {earlyPending=false;}
    bool Ready(std::uint64_t now) {
        if(!pending) return false;
        if(!readySince) readySince=now;
        return now-readySince>=(checks?5000u:2000u);
    }
    void Checked() {if(++checks>=2) pending=false;}
};
inline void Validate(const AutoOutfitSettings& s) {
    if(s.delaySeconds<0 || s.delaySeconds>10 || s.columns<3 || s.columns>5 ||
       s.sneakDelaySeconds<0 || s.sneakDelaySeconds>5 || s.combatIntervalSeconds<2 || s.combatIntervalSeconds>10 ||
       s.ostimRecoverySeconds<5 || s.ostimRecoverySeconds>30) throw std::runtime_error("Invalid follower automatic settings");
    for(const auto& p:s.slots) if(!p.empty() && (p.size()>240 || p.find_first_of("/\\:")!=std::string::npos ||
        p.find('\0')!=std::string::npos || std::filesystem::path(p).extension()!=".png")) throw std::runtime_error("Invalid follower outfit photo");
    for(unsigned i=23;i<=25;++i) if(!s.slots[i].empty()) throw std::runtime_error("Follower crafting outfits are not supported");
}
inline std::string Encode(const AutoOutfitSettings& s) {
    Validate(s);
    return nlohmann::json{{"slots",s.slots},{"delay",s.delaySeconds},{"columns",s.columns},{"recovery",s.ostimRecoverySeconds},
        {"enabled",s.enabled},{"paused",s.paused},{"combat",s.combatEnabled},{"hide",s.hideAssignButtons},
        {"sneak",s.sneakDelaySeconds},{"interval",s.combatIntervalSeconds}}.dump();
}
inline AutoOutfitSettings Decode(const std::string& bytes) {
    if(bytes.size()>8192) throw std::runtime_error("Follower automatic settings too large");
    const auto j=nlohmann::json::parse(bytes);
    for(auto key:{"enabled","paused","combat","hide"}) if(!j.at(key).is_boolean()) throw std::runtime_error("Invalid follower toggle");
    for(auto key:{"delay","columns","recovery","sneak","interval"})
        if(!j.at(key).is_number_integer() || j.at(key)<0 || j.at(key)>30) throw std::runtime_error("Invalid follower setting");
    AutoOutfitSettings s;
    if(!j.at("slots").is_array() || j.at("slots").size()!=s.slots.size()) throw std::runtime_error("Invalid follower slots");
    s.slots=j.at("slots").get<AutoOutfitSlots>();
    s.delaySeconds=j.at("delay"); s.columns=j.at("columns"); s.ostimRecoverySeconds=j.at("recovery");
    s.enabled=j.at("enabled"); s.paused=j.at("paused"); s.combatEnabled=j.at("combat"); s.hideAssignButtons=j.at("hide");
    s.sneakDelaySeconds=j.at("sneak"); s.combatIntervalSeconds=j.at("interval"); Validate(s); return s;
}
inline bool CanRegister(std::size_t count,bool exists) {return exists || count<Limit;}
// One mutation at a time, even when player and followers become due together.
inline bool MayApply(std::uint64_t now,std::uint64_t next,bool verifying) {return !verifying && now>=next;}
inline bool PollPlayer(std::uint64_t now,std::uint64_t next,bool beforeFollowers) {return beforeFollowers || now>=next;}
}
