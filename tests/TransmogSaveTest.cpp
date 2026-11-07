#include "TransmogSave.h"
#include <iostream>
using namespace Gallery::TransmogSave;
void require(bool v){if(!v) throw std::runtime_error("Transmog persistence regression");}
template<class F> void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}require(bad);}
int main(){try {
    State full{0x14,0x2005A184,UINT32_MAX,true,false,true,{0xFE100800,0xFE100801}};
    require(Decode(Encode(full))==full);
    auto partial=full;partial.scope=0x1803;partial.autoOwned=true;require(Decode(Encode(partial))==partial);
    State modeOnly;modeOnly.actor=0xFF001216;modeOnly.mode=true;require(Decode(Encode(modeOnly))==modeOnly);
    auto mapped=full;require(Remap(mapped,[](auto old,auto& current){current=old+1;return true;}));
    require(mapped.actor==full.actor+1 && mapped.race==full.race+1 && mapped.armor[1]==full.armor[1]+1);
    mapped=full;require(!Remap(mapped,[](auto,auto&){return false;}));
    mapped=full;require(Remap(mapped,[&](auto old,auto& now){now=old;return old!=full.armor[1];}));
    require(mapped.armor.empty() && mapped.mode && !mapped.race && !mapped.autoOwned && Valid(mapped));
    mapped=full;require(Remap(mapped,[&](auto old,auto& now){now=old;return old!=full.race;}));require(mapped.armor.empty());
    mapped=modeOnly;unsigned calls{};require(Remap(mapped,[&](auto old,auto& now){++calls;now=old;return true;}));require(calls==1);
    auto invalid=full;invalid.armor.push_back(invalid.armor[0]);rejects([&]{Encode(invalid);});
    invalid=full;invalid.armor.resize(MaxArmors+1,1);rejects([&]{Encode(invalid);});
    invalid=full;invalid.scope=0;rejects([&]{Encode(invalid);});
    rejects([]{Decode("{");});rejects([]{Decode(std::string(MaxBytes+1,' '));});
    auto j=nlohmann::json::parse(Encode(full));j["actor"]=-1;rejects([&]{Decode(j.dump());});
    j["actor"]=UINT64_MAX;rejects([&]{Decode(j.dump());});
    j=nlohmann::json::parse(Encode(full));j["mode"]=1;rejects([&]{Decode(j.dump());});
    j=nlohmann::json::parse(Encode(full));j.erase("race");rejects([&]{Decode(j.dump());});
    State reset;reset.actor=0x14;require(Decode(Encode(reset))==reset);
    std::cout<<"PASS: exact full/partial/mode-only state, dynamic IDs, remap, missing-gear isolation, bounds and corrupt records\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
