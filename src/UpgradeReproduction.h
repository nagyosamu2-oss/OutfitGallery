#pragma once
#include <atomic>
#include <cstdint>
namespace Gallery {
// 0 is undecided, 1 base properties, 2 saved upgrades. Choice belongs to the co-save.
inline std::atomic<unsigned> upgradeChoice{};
inline std::atomic<bool> defaultUpgradeReproduction{true},skipUpgradePrompt{false};
inline bool ValidUpgradeChoice(unsigned value){return value==1 || value==2;}
inline void ResetUpgradeChoice(){upgradeChoice=0;}
inline void ChooseUpgradeMode(bool reproduce){upgradeChoice=reproduce?2u:1u;}
inline unsigned ResolveUpgradeChoice(){
 unsigned choice=upgradeChoice.load();
 if(!choice && skipUpgradePrompt.load()) {
  const unsigned desired=defaultUpgradeReproduction.load()?2u:1u;
  upgradeChoice.compare_exchange_strong(choice,desired);
  choice=upgradeChoice.load();
 }
 return choice;
}
// Before the first answer, missing-item generation uses base properties.
inline bool RestoreSavedUpgrades(){return ResolveUpgradeChoice()==2;}
inline bool NeedsUpgradePrompt(){return ResolveUpgradeChoice()==0;}
}
