#include "MissingEnchantmentPolicy.h"
#include <cassert>
int main(){using namespace Gallery;
 Preset source;source.items={{"Skyrim.esm",1,"Boots","armor",""},{"Armor.esp",2,"Top","armor",""}};
 source.items[0].enchantment="recipe";source.items[0].preferred={true,1234,2,"enchanted"};source.items[0].smithing={1.5f,1234,2};
 std::string omitted;auto result=WithoutMissingEnchantments(source,[](const Item&){throw MissingEnchantmentReference("unavailable","SCBarrels.esp");},&omitted);
 assert(result.items.size()==2 && result.items[0].enchantment.empty() && !result.items[0].preferred.custom);
 assert(result.items[0].smithing==source.items[0].smithing && result.items[1].name=="Top");
 assert(source.items[0].enchantment=="recipe" && source.items[0].preferred.custom && omitted=="Boots [SCBarrels.esp]");
 auto restored=WithoutMissingEnchantments(source,[](const Item&){});assert(restored.items[0].enchantment=="recipe"); // restored provider or usable owned instance
 bool threw=false;try{WithoutMissingEnchantments(source,[](const Item&){throw std::runtime_error("malformed recipe");});}catch(const std::runtime_error&){threw=true;}assert(threw);
 Preset ordinary;ordinary.items={source.items[1]};int calls=0;WithoutMissingEnchantments(ordinary,[&](const Item&){++calls;});assert(calls==0);
}
