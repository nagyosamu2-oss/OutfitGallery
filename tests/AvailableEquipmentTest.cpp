#include "AvailableEquipment.h"
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){using namespace Gallery;
 Preset source;source.name="mixed";source.items={{"Present.esp",1,"boots","armor",""},{"Missing.esp",2,"ring","armor",""},{"Present.esp",3,"prop","weapon",""}};
 source.items[0].enchantment="recipe";source.items[0].smithing.health=1.5f;
 auto resolve=[](const Item& i)->std::optional<std::uint32_t>{if(i.plugin=="Missing.esp")return std::nullopt;return i.localID==4?0u:0x80u;};
 std::string skipped;auto p=FilterUnavailableEquipment(source,resolve,&skipped);
 check(p.items.size()==1 && p.items[0].enchantment=="recipe" && p.items[0].smithing.health==1.5f);
 check(skipped=="Missing.esp / ring" && source.items.size()==3 && !source.accessories);
 check(FilterUnavailableEquipment(source,[](const Item&)->std::optional<std::uint32_t>{return 0x80u;}).items.size()==2); // plugin returns
 source.accessories=true;source.slotMask=0xC0; p=FilterUnavailableEquipment(source,resolve);check(p.slotMask==0x80 && source.slotMask==0xC0);
 source.items.push_back({"Present.esp",4,"slotless","armor",""});p=FilterUnavailableEquipment(source,resolve);check(p.items.size()==2 && p.slotMask==0x80);
 bool threw=false;try{FilterUnavailableEquipment(source,[](const Item&)->std::optional<std::uint32_t>{return std::nullopt;});}catch(...){threw=true;}check(threw);
 threw=false;try{FilterUnavailableEquipment(source,[](const Item&)->std::optional<std::uint32_t>{throw std::runtime_error("changed type");});}catch(...){threw=true;}check(threw);
}
