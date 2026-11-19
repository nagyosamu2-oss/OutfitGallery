#pragma once
#include "Presets.h"
namespace Gallery {
// Filter only a working copy. A null mask means the base form is absent.
// The resolver must throw for changed types/invalid slot layouts.
template<class ResolveMask>
Preset FilterUnavailableEquipment(const Preset& source,ResolveMask resolve,std::string* skipped=nullptr) {
 auto result=ClothingPreset(source);result.items.clear();std::uint32_t masks{};bool absent=false;
 for(const auto& item:source.items)if(item.kind=="armor") {
  const auto mask=resolve(item);
  if(!mask) {absent=true;if(skipped){if(!skipped->empty())*skipped+=", ";*skipped+=item.plugin+" / "+item.name;}continue;}
  result.items.push_back(item);masks|=*mask;
 }
 if(result.items.empty())throw std::runtime_error("No available equipment in this set. Equipment unchanged.");
 // Accessory verification/removal must target only surviving accessory slots.
 if(absent && result.accessories)result.slotMask=masks;
 return result;
}
}
