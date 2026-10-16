#include "RegisteredName.h"
#include "Presets.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void Require(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
 // Exhaustively exercise all combinations of the independent protection flags.
 for(unsigned flags=0;flags<64;++flags) for(bool compatibility:{false,true}) {
  const bool quest=flags&1, tempered=flags&2, enchanted=flags&4;
  const bool named=flags&8, favorite=flags&16, poisoned=flags&32;
  const bool protectedResult=ProtectGeneratedItem(compatibility,quest,tempered,enchanted,named,favorite,poisoned);
  if(flags & ~8u) Require(protectedResult,"non-name protections always survive compatibility");
  else Require(protectedResult==(named && !compatibility),"only name protection may be bypassed");
 }
 const RegisteredName n{"Silver Ring","Silver Ring [LT]"};
 Require(MatchesRegisteredName(true,true,n,n.base,n.display,true),"exact saved name allowed");
 Require(!MatchesRegisteredName(false,true,n,n.base,n.display,true),"OFF protects names");
 Require(!MatchesRegisteredName(true,false,n,n.base,n.display,true),"preowned/untracked protected");
 Require(!MatchesRegisteredName(true,true,{},n.base,n.display,true),"legacy has no evidence");
 Require(!MatchesRegisteredName(true,true,n,n.base,"Silver Ring [LI]",true),"changed finger protected");
 Require(!MatchesRegisteredName(true,true,n,n.base,"My Ring",true),"user rename protected");
 Require(!MatchesRegisteredName(true,true,n,"Other Ring",n.display,true),"base mismatch protected");
 Require(!MatchesRegisteredName(true,true,n,n.base,n.display,false),"quest/message/temper name protected");
 Require(!ValidRegisteredName({n.base,std::string(513,'x')}),"oversized name rejected");
 Require(!ValidRegisteredName({n.base,std::string("a\0b",3)}),"embedded NUL rejected");
 auto dir=std::filesystem::temp_directory_path()/"OutfitGalleryRegisteredNameTest";
 std::filesystem::create_directories(dir);
 Preset p{"Test","test.png",{{"Skyrim.esm",0x3b97c,"Silver Ring","armor",""}}};
 p.items[0].registeredName=n;
 auto path=dir/"name.json"; std::filesystem::remove(path); WritePreset(p,path);
 const auto restored=ReadPreset(path);
 Require(restored.items[0].registeredName==n,"registered name roundtrip");
 Require(SameEquipment(p,restored),"evidence must not change equip identity");
 nlohmann::json j;{std::ifstream in(path);in>>j;}
 auto read=[&](const nlohmann::json& data){std::ofstream out(path);out<<data;out.close();return ReadPreset(path);};
 auto legacy=j;legacy["items"][0].erase("registeredName");
 Require(read(legacy).items[0].registeredName.display.empty(),"old presets supported conservatively");
 for(auto bad:{nlohmann::json(nullptr),nlohmann::json("bad"),nlohmann::json{{"version",2},{"base",n.base},{"display",n.display}},nlohmann::json{{"version",1},{"base",n.base},{"display",std::string(513,'x')}}}) {
  auto broken=j;broken["items"][0]["registeredName"]=bad;
  Require(read(broken).items[0].registeredName.display.empty(),"invalid metadata grants no exception");
 }
 std::filesystem::remove(path);std::filesystem::remove(dir);
 std::cout<<"Registered-name safety and persistence scenarios passed\n";
}
