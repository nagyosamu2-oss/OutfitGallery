#pragma once
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string>
#include <stdexcept>
namespace Gallery::EnchantProbe {
using Json=nlohmann::json;
inline constexpr std::size_t MaxEffects=32;
inline void Require(bool v,const char* why) {if(!v)throw std::runtime_error(why);}
inline std::uint32_t UInt(const Json& j,std::uint32_t max=0xFFFFFFFFu) {
 Require(j.is_number_integer(),"Expected unsigned integer");
 if(j.is_number_unsigned()) {auto v=j.get<std::uint64_t>();Require(v<=max,"Integer out of range");return static_cast<std::uint32_t>(v);}
 auto v=j.get<std::int64_t>();Require(v>=0 && static_cast<std::uint64_t>(v)<=max,"Integer out of range");return static_cast<std::uint32_t>(v);
}
inline std::int32_t SInt(const Json& j) {
 Require(j.is_number_integer(),"Expected signed integer");
 if(j.is_number_unsigned()) {auto v=j.get<std::uint64_t>();Require(v<=0x7FFFFFFFu,"Signed integer out of range");return static_cast<std::int32_t>(v);}
 auto v=j.get<std::int64_t>();Require(v>=-2147483648LL && v<=2147483647LL,"Signed integer out of range");return static_cast<std::int32_t>(v);
}
template<class F> inline void Field(const char* name,F validate) {
 try {validate();}catch(const std::exception& e){throw std::runtime_error(std::string(name)+": "+e.what());}
}
inline float FloatBits(const Json& j) {auto f=std::bit_cast<float>(UInt(j));Require(std::isfinite(f),"Non-finite recipe value");return f;}
inline void Key(const Json& j,bool nullable=false) {
 if(nullable && j.is_null())return;
 Require(j.is_array() && j.size()==2 && j[0].is_string(),"Invalid portable form reference");
 auto s=j[0].get<std::string>();
 Require(!s.empty() && s.size()<=240 && s.find_first_of("/\\:")==std::string::npos && s.find('\0')==std::string::npos,"Invalid plugin filename");
 Require(s.ends_with(".esm") || s.ends_with(".esp") || s.ends_with(".esl"),"Invalid plugin extension");
 Require(UInt(j[1],0xFFFFFFu)>0,"Invalid local form ID");
}
inline void Signature(const Json& j) {
 Require(j.is_array() && j.size()==14 && UInt(j[0])==1,"Unknown enchantment signature version");
 Require(j[1].is_array() && !j[1].empty() && j[1].size()<=MaxEffects,"Enchantment reproduction supports 1 to 32 effects");
 for(const auto& e:j[1]) {
  Require(e.is_array() && e.size()==5,"Invalid effect");Key(e[0]);
  Require(std::abs(FloatBits(e[1]))<=1000000.f,"Magnitude outside probe limits");
  Field("effect area",[&]{UInt(e[2]);});
  Field("effect duration",[&]{UInt(e[3]);});
  Field("effect cost",[&]{auto cost=FloatBits(e[4]);Require(cost>=0 && cost<=1000000000.f,"Invalid effect cost");});
 }
 Field("costOverride",[&]{SInt(j[2]);});
 Field("flags",[&]{UInt(j[3]);});
 Field("chargeOverride",[&]{SInt(j[5]);});
 Require(UInt(j[4])==0 && UInt(j[6])==0 && UInt(j[7])==6,"Probe requires constant/self armor enchantment");
 Require(FloatBits(j[8])==0.f,"Nonzero charge time unsupported");
 Key(j[9],true);Key(j[10],true);UInt(j[11],65535);
 Require(j[12].is_boolean() && !j[12].get<bool>(),"Temporary enchantment unsupported");
 auto health=FloatBits(j[13]);Require(health>=1.f && health<=1000.f,"Smithing outside probe limits");
}
inline Json ParseSignature(const std::string& text) {
 Require(!text.empty() && text.size()<=16384,"Enchantment recipe size invalid");
 auto j=Json::parse(text);Signature(j);return j;
}
// The engine factory can rebuild the same effects without the crafting UI's
// gold-value override. Never patch that field on a potentially shared ENCH.
inline Json PerformanceSignature(const std::string& text,bool ignoreHealth) {
 auto j=ParseSignature(text);
 const auto flags=UInt(j[3]);
 // The active override must have a usable nonnegative cost. Other flags remain exact.
 Require(!(flags&1u) || SInt(j[2])>=0,"Invalid active enchantment cost override");
 j[2]=-1;j[3]=flags&~1u;
 if(ignoreHealth)j[13]=std::bit_cast<std::uint32_t>(1.f);
 return j;
}
inline bool FactoryEquivalent(const std::string& expected,const std::string& actual) {
 try {return PerformanceSignature(expected,false)==PerformanceSignature(actual,false);}
 catch(const std::exception&){return false;}
}
inline bool SameEnchant(const std::string& expected,const std::string& actual) {
 if(expected.empty() || actual.empty())return expected.empty() && actual.empty();
 try {return PerformanceSignature(expected,true)==PerformanceSignature(actual,true);}
 catch(const std::exception&){return false;}
}
inline Json Read(const std::string& text) {
 Require(!text.empty() && text.size()<=32768,"Sample file too large or empty");
 auto j=Json::parse(text);
 Require(j.is_object() && UInt(j.at("schema"))==1,"Unknown probe sample version");
 Key(j.at("armor"));Require(j.at("name").is_string() && j.at("name").get_ref<const std::string&>().size()<=1024,"Invalid armor name");
 Signature(j.at("enchantment"));return j;
}
}
