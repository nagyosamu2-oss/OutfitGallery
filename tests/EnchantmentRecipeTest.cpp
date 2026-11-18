#include "EnchantmentRecipe.h"
#include <cstdlib>
using namespace Gallery::EnchantProbe;
void check(bool b){if(!b)std::abort();}
void reject(Json j){try{Read(j.dump());std::abort();}catch(const std::exception&) {}}
int main(){
 auto bits=[](float f){return std::bit_cast<std::uint32_t>(f);};
 Json sig={1,Json::array({Json::array({Json::array({"Skyrim.esm",0x1234}),bits(20.f),0,0,bits(10.f)})}),0,0,0,0,0,6,bits(0.f),nullptr,nullptr,0,false,bits(1.1f)};
 Json j={{"schema",1},{"armor",{"Armor.esl",0x800}},{"name","Test"},{"enchantment",sig}};
 // Cleanup compares all enchantment metadata but handles smithing separately.
 auto healthChanged=sig;healthChanged[13]=bits(1.5f);
 check(SameEnchant(sig.dump(),healthChanged.dump()));
 auto effectChanged=sig;effectChanged[1][0][1]=bits(21.f);
 check(!SameEnchant(sig.dump(),effectChanged.dump()));
 check(!SameEnchant(sig.dump(),""));check(!SameEnchant(sig.dump(),"broken"));
 auto addedEffect=sig;addedEffect[1].push_back(sig[1][0]);
 check(!SameEnchant(sig.dump(),addedEffect.dump()));
 // Recorded probe2 success: two effects, health 1.5, signed chargeOverride=-1.
 Json observed=Json::parse(R"([1,[[["Skyrim.esm",499956],1084227584,0,0,1122692222],[["Skyrim.esm",499965],1087722838,0,0,1098969874]],442,1,0,-1,0,6,0,null,null,1000,false,1069547520])");
 check(ParseSignature(observed.dump())==observed);check(FloatBits(observed[13])==1.5f);
 // Actual restart/new-game mismatch: effects unchanged; UI-set price becomes automatic.
 Json saved=Json::parse(R"([1,[[["Skyrim.esm",499956],1084227584,0,0,1122692222],[["Skyrim.esm",499965],1087722838,0,0,1098969874]],438,1,0,-1,0,6,0,null,null,1000,false,1066192077])");
 auto automatic=saved;automatic[2]=-1;automatic[3]=0;
 check(FactoryEquivalent(saved.dump(),automatic.dump()));check(FactoryEquivalent(automatic.dump(),saved.dump()));
 check(SameEnchant(saved.dump(),automatic.dump()));
 auto otherPrice=saved;otherPrice[2]=442;check(FactoryEquivalent(saved.dump(),otherPrice.dump()));
 auto modified=saved;modified[1][0][1]=bits(6.f);check(!FactoryEquivalent(saved.dump(),modified.dump()));check(!SameEnchant(saved.dump(),modified.dump()));
 modified=saved;modified[13]=bits(1.5f);check(!FactoryEquivalent(saved.dump(),modified.dump()));check(SameEnchant(saved.dump(),modified.dump()));
 for(int n:{4,6,7,11}) {modified=saved;modified[n]=1;check(!FactoryEquivalent(saved.dump(),modified.dump()));}
 modified=saved;modified[3]=9;check(!FactoryEquivalent(saved.dump(),modified.dump()));
 modified=saved;modified[9]={"Skyrim.esm",0x1234};check(!FactoryEquivalent(saved.dump(),modified.dump()));
 modified=saved;modified[10]={"Skyrim.esm",0x1234};check(!FactoryEquivalent(saved.dump(),modified.dump()));
 modified=saved;modified[2]=-1;check(!FactoryEquivalent(saved.dump(),modified.dump())); // active invalid override
 modified=saved;modified[12]=true;check(!FactoryEquivalent(saved.dump(),modified.dump()));
 check(!FactoryEquivalent(saved.dump(),"broken"));check(!SameEnchant(saved.dump(),""));
 auto round=Read(j.dump());check(round==j);check(FloatBits(round["enchantment"][13])==1.1f);
 auto two=j;two["enchantment"][1].push_back(sig[1][0]);check(Read(two.dump())==two);

 // Engine ENIT override fields are signed, including sentinel -1; flags are uint32.
 auto signedSample=j;signedSample["enchantment"][2]=-1;signedSample["enchantment"][5]=-1;
 check(Read(signedSample.dump())==signedSample);
 auto limits=signedSample;limits["enchantment"][2]=-2147483648LL;limits["enchantment"][5]=2147483647;limits["enchantment"][3]=0xFFFFFFFFu;
 check(Read(limits.dump())==limits);
 auto overflow=j;overflow["enchantment"][2]=2147483648ULL;reject(overflow);
 overflow=j;overflow["enchantment"][5]=-2147483649LL;reject(overflow);
 overflow=j;overflow["enchantment"][3]=-1;reject(overflow);
 overflow=j;overflow["enchantment"][3]=4294967296ULL;reject(overflow);
 try {SInt(Json(2147483648ULL));std::abort();}catch(const std::exception& e){check(std::string(e.what()).find("Signed integer")!=std::string::npos);}
 auto k=j;k["schema"]=2;reject(k);k=j;k["armor"][0]="../Armor.esp";reject(k);
 k=j;k["armor"][1]=-1;reject(k);k=j;k["armor"][1]=0xFF000001u;reject(k);
 k=j;k["enchantment"][1][0][1]=0x7FC00000u;reject(k);
 k=j;k["enchantment"][1][0][1]=-1;reject(k);
 k=j;k["enchantment"][1][0][2]=1;check(Read(k.dump())==k);
 k=j;k["enchantment"][1][0][3]=1;check(Read(k.dump())==k);
 auto sized=j;sized["enchantment"][1][0][2]=0xFFFFFFFFu;sized["enchantment"][1][0][3]=0xFFFFFFFFu;
 check(Read(sized.dump())==sized);
 for(int field:{2,3}) {
  auto invalid=j;invalid["enchantment"][1][0][field]=-1;reject(invalid);
  invalid=j;invalid["enchantment"][1][0][field]=4294967296ULL;reject(invalid);
  auto modified=sig;modified[1][0][field]=1;
  check(!FactoryEquivalent(sig.dump(),modified.dump()));check(!SameEnchant(sig.dump(),modified.dump()));
 }
 k=j;k["enchantment"][1].clear();reject(k);
 k=two;k["enchantment"][1].push_back(sig[1][0]);check(Read(k.dump())==k);
 auto many=j;many["enchantment"][1].clear();
 for(unsigned n=0;n<32;++n) {auto effect=sig[1][0];effect[1]=bits(float(n+1));many["enchantment"][1].push_back(effect);}
 check(Read(many.dump())==many);check(ParseSignature(many["enchantment"].dump())==many["enchantment"]);
 auto overflowCount=many;overflowCount["enchantment"][1].push_back(sig[1][0]);reject(overflowCount);
 auto badLast=many;badLast["enchantment"][1][31][1]=0x7FC00000u;reject(badLast);
 auto modifiedLast=many;modifiedLast["enchantment"][1][31][1]=bits(99.f);
 check(!FactoryEquivalent(many["enchantment"].dump(),modifiedLast["enchantment"].dump()));
 check(!SameEnchant(many["enchantment"].dump(),modifiedLast["enchantment"].dump()));
 auto reordered=many;std::swap(reordered["enchantment"][1][0],reordered["enchantment"][1][31]);
 check(!FactoryEquivalent(many["enchantment"].dump(),reordered["enchantment"].dump()));
 for(int n:{4,6,7}){k=j;k["enchantment"][n]=9;reject(k);}
 k=j;k["enchantment"][12]=true;reject(k);
 k=j;k["enchantment"][13]=bits(.5f);reject(k);
 k=j;k["enchantment"][11]=65536;reject(k);
 k=j;k["enchantment"][1][0][0][0]="dynamic";reject(k);
 k=j;k["enchantment"][9]={"Ench.esm",0x1234};check(Read(k.dump())==k);
 try{Read(std::string(32769,' '));std::abort();}catch(const std::exception&){}
}
