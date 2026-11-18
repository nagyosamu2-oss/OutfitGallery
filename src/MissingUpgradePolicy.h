#pragma once
#include "Presets.h"
namespace Gallery {
// Apply to a temporary preset only. Stored recipe data is never discarded.
inline void UseBasePropertiesIfMissing(Item& item,bool knownOwnedRecipe) {
 if(knownOwnedRecipe)return;
 item.smithing={};item.enchantment.clear();item.preferred={};
}
}
