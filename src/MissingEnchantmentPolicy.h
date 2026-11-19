#pragma once
#include "Presets.h"
#include "PortableEnchantment.h"
namespace Gallery {
// Only unavailable portable references permit fallback. Never edit the source,
// remove owned enchantments, truncate effects or swallow unrelated errors.
template<class Validate>
Preset WithoutMissingEnchantments(const Preset& source,Validate validate,std::string* omitted=nullptr) {
 auto result=source;
 for(auto& item:result.items)if(!item.enchantment.empty()) {
  try{validate(item);}
  catch(const MissingEnchantmentReference& e) {
   if(omitted){if(!omitted->empty())*omitted+=", ";*omitted+=item.name+" ["+e.reference+"]";}
   item.enchantment.clear();item.preferred={};
  }
 }
 return result;
}
}
