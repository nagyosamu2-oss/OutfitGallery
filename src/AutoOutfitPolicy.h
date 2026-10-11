#pragma once
#include <array>
#include <string>
#include <cstdint>
namespace Gallery::AutoOutfit {
enum class Place : unsigned { Normal, Town, Home, Inn, Combat, Night, Rain, Snow, Dungeon, Temple, Swimming, Dawnguard, College, Brotherhood, Companions, Bards, Thieves, BluePalace, Melee, Magic, Archery, Unarmed, Sneaking, Smithing, Alchemy, Enchanting, Shop };
using Slots=std::array<std::string,27>;
inline bool IsCombat(Place p) {return p==Place::Combat || (p>=Place::Melee && p<=Place::Unarmed);}
inline bool HasCombatStyles(const Slots& slots) {
    for(unsigned i=18;i<=static_cast<unsigned>(Place::Unarmed);++i) if(!slots[i].empty()) return true;
    return false;
}
enum class Hand {Empty, Melee, Magic, Archery, Other};
inline Place CombatStyle(Hand right,Hand left) {
    // Two-handed ranged weapons may be exposed through either hand.
    if(right==Hand::Archery || left==Hand::Archery) return Place::Archery;
    if(right==Hand::Melee) return Place::Melee;
    if(right==Hand::Magic) return Place::Magic;
    if(right==Hand::Empty && left==Hand::Empty) return Place::Unarmed;
    return Place::Combat;
}
struct StyleDebounce {
    int candidate=-1;
    std::uint64_t since{};
    void Block() {candidate=-1;}
    bool Observe(Place style,std::uint64_t now) {
        const int next=static_cast<int>(style);
        if(candidate!=next) {candidate=next;since=now;}
        return now-since>=(style==Place::Unarmed?2000u:1000u);
    }
};
inline Place Classify(bool home,bool inn,bool town,bool dungeon=false,bool temple=false,bool interior=true,bool shop=false) {
    return home?Place::Home:inn?Place::Inn:temple?Place::Temple:(dungeon && interior)?Place::Dungeon:(shop && interior)?Place::Shop:town?Place::Town:Place::Normal;
}
inline bool IsGuild(Place p) {return p>=Place::Dawnguard && p<=Place::BluePalace;}
struct Environment {bool outside{}, night{}, rain{}, snow{}, swimming{}, sneaking{};};
// Water entry is opt-in by assignment. Do not relax ordinary gallery entry.
inline bool SwimmingChangeAllowed(bool swimming,Place condition,const Slots& slots) {
    return !swimming || (!slots[10].empty() && (condition==Place::Swimming || IsCombat(condition)));
}
inline bool IsNight(float hour) {return hour>=0.f && hour<24.f && (hour>=20.f || hour<6.f);}
inline Place SelectCondition(Place location,const Environment& env,bool combat,const Slots& slots,Place guild=Place::Normal) {
    if(combat) return Place::Combat; // Combat enable/unassigned handling remains a separate gate.
    if(env.swimming && !slots[10].empty()) return Place::Swimming;
    if(env.sneaking && !slots[22].empty()) return Place::Sneaking;
    if(!env.outside && IsGuild(guild) && !slots[static_cast<unsigned>(guild)].empty()) return guild;
    // Home/inn/temple can cover exteriors. Dungeon classification requires an interior.
    if(location==Place::Home || location==Place::Inn || location==Place::Dungeon || location==Place::Temple || location==Place::Shop) return location;
    if(env.outside) {
        if(env.snow && !slots[7].empty()) return Place::Snow;
        if(env.rain && !slots[6].empty()) return Place::Rain;
        if(env.night && !slots[5].empty()) return Place::Night;
    }
    return location;
}
inline int ChooseSlot(Place place,const Slots& slots,bool inTown) {
    if(IsCombat(place)) {
        const auto i=static_cast<unsigned>(place);
        return !slots[i].empty()?static_cast<int>(i):slots[4].empty()?-1:4;
    }
    if(place==Place::Enchanting || place==Place::Alchemy || place==Place::Smithing || place==Place::Sneaking || IsGuild(place) || place==Place::Swimming || place==Place::Combat || place==Place::Night || place==Place::Rain || place==Place::Snow) {
        const auto i=static_cast<unsigned>(place);return slots[i].empty()?-1:static_cast<int>(i);
    }
    if(place==Place::Shop && !slots[26].empty()) return 26;
    if(place==Place::Dungeon && !slots[8].empty()) return 8;
    if(place==Place::Temple && !slots[9].empty()) return 9;
    if(place==Place::Home && !slots[2].empty()) return 2;
    if(place==Place::Inn && !slots[3].empty()) return 3;
    if((place==Place::Town || place==Place::Home || place==Place::Inn || inTown) && !slots[1].empty()) return 1;
    return slots[0].empty()?-1:0;
}
inline std::string Choose(Place place,const Slots& slots,bool inTown) {
    const int i=ChooseSlot(place,slots,inTown);return i<0?std::string{}:slots[i];
}
inline std::uint64_t ConditionDelay(Place condition,std::uint64_t configured,std::uint64_t sneak=2000) {
    return condition==Place::Sneaking?sneak:configured;
}
// Pure transition policy: never reassert an outfit on an unchanged condition.
struct Policy {
    int candidate=-1, stable=-1, manual=-1;
    std::uint64_t since{}, lastAttempt{};
    bool handled{};
    std::string lastPhoto;
    void Block() {candidate=-1;}
    void Manual(int place) {manual=place; stable=place; handled=true; lastPhoto.clear(); candidate=-1;}
    bool Observe(int place,std::uint64_t now,std::uint64_t delayMs=3000,std::uint64_t combatIntervalMs=5000) {
        if(candidate!=place) {candidate=place; since=now;}
        if(now-since<delayMs) return false;
        if(stable!=place) {stable=place; handled=false; if(manual!=place) manual=-1;}
        return manual!=place && !handled && (!lastAttempt || now-lastAttempt>=(IsCombat(static_cast<Place>(place))?combatIntervalMs:5000));
    }
    bool Attempt(const std::string& photo,std::uint64_t now) {
        handled=true; lastAttempt=now;
        if(photo.empty() || photo==lastPhoto) return false;
        return true;
    }
    void Applied(const std::string& photo) {lastPhoto=photo;}
};
// Observe raw combat state even while paused/blocked. Location changes, temporary
// blockers and configuration edits must not rearm an already consumed battle.
struct CombatSession {
    bool active{}, attempted{}, manualOverride{};
    int attemptedStyle=-1;
    bool Observe(bool fighting,Policy& policy) {
        if(active==fighting) return false;
        active=fighting; attempted=false; manualOverride=false; attemptedStyle=-1;
        if(IsCombat(static_cast<Place>(policy.candidate))) policy.candidate=-1;
        if(IsCombat(static_cast<Place>(policy.stable))) {
            policy.stable=-1; policy.manual=-1; policy.handled=false;
        }
        // Preserve the global attempt cooldown and applied-photo cache.
        return true;
    }
    bool Allowed(bool enabled,bool assigned,int style=-1) const {
        return active && enabled && assigned && !manualOverride && (!attempted || (style>=0 && style!=attemptedStyle));
    }
    void Consume(int style=-1) {attempted=true;attemptedStyle=style;}
    void Manual(Policy& policy) {
        Observe(true,policy); Consume(); manualOverride=true; policy.Manual(static_cast<int>(Place::Combat));
    }
};
}
