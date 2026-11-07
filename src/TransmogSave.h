#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <nlohmann/json.hpp>
namespace Gallery::TransmogSave {
inline constexpr std::uint32_t Record=0x544D4F47, Version=1;
inline constexpr std::size_t MaxActors=64, MaxArmors=256, MaxBytes=8192;
struct State {
    std::uint32_t actor{},race{},scope{};
    bool mode{},autoOwned{},female{};
    std::vector<std::uint32_t> armor;
    bool operator==(const State&) const = default;
};
inline bool Valid(const State& s) {
    if(!s.actor || s.armor.size()>MaxArmors) return false;
    if(!s.race) return s.armor.empty() && !s.scope && !s.autoOwned && !s.female;
    if(!s.race || !s.scope) return false;
    std::set<std::uint32_t> ids;
    for(auto id:s.armor) if(!id || !ids.insert(id).second) return false;
    return true;
}
inline void DropLook(State& s) {s.race=0;s.scope=0;s.autoOwned=false;s.female=false;s.armor.clear();}
inline std::string Encode(const State& s) {
    if(!Valid(s)) throw std::runtime_error("Invalid saved appearance");
    const auto text=nlohmann::json{{"actor",s.actor},{"race",s.race},{"scope",s.scope},
        {"mode",s.mode},{"auto",s.autoOwned},{"female",s.female},{"armor",s.armor}}.dump();
    if(text.size()>MaxBytes) throw std::runtime_error("Saved appearance too large");
    return text;
}
inline State Decode(const std::string& text) {
    if(text.empty() || text.size()>MaxBytes) throw std::runtime_error("Invalid appearance length");
    const auto j=nlohmann::json::parse(text);
    auto id=[&](const nlohmann::json& v) -> std::uint32_t {
        if(!v.is_number_unsigned() || v.get<std::uint64_t>()>UINT32_MAX) throw std::runtime_error("Invalid form ID");
        return v.get<std::uint32_t>();
    };
    if(!j.is_object() || !j.at("mode").is_boolean() || !j.at("auto").is_boolean() || !j.at("female").is_boolean() ||
       !j.at("armor").is_array() || j.at("armor").size()>MaxArmors) throw std::runtime_error("Invalid appearance fields");
    State s;s.actor=id(j.at("actor"));s.race=id(j.at("race"));s.scope=id(j.at("scope"));
    s.mode=j.at("mode").get<bool>();s.autoOwned=j.at("auto").get<bool>();s.female=j.at("female").get<bool>();
    for(const auto& v:j.at("armor")) s.armor.push_back(id(v));
    if(!Valid(s)) throw std::runtime_error("Invalid appearance state");
    return s;
}
// Mandatory SKSE form remapping; unresolved gear discards the WHOLE look rather
// than restoring a partial body, while preserving the user's selected mode.
template<class Resolve> bool Remap(State& s,Resolve resolve) {
    std::uint32_t current{};
    if(!resolve(s.actor,current) || !current) return false;
    s.actor=current;
    if(!s.race) return true;
    if(!resolve(s.race,current) || !current) {DropLook(s);return true;}
    s.race=current;
    for(auto& armor:s.armor) {
        if(!resolve(armor,current) || !current) {DropLook(s);return true;}
        armor=current;
    }
    if(!Valid(s)) DropLook(s);
    return true;
}
}
