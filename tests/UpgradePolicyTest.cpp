#include "UpgradeReproduction.h"
#include "MissingUpgradePolicy.h"
#include <cstdlib>
void check(bool value){if(!value)std::abort();}
int main(){using namespace Gallery;
 skipUpgradePrompt=false;defaultUpgradeReproduction=true;ResetUpgradeChoice();
 check(NeedsUpgradePrompt());check(!RestoreSavedUpgrades());check(ResolveUpgradeChoice()==0);
 ChooseUpgradeMode(true);check(!NeedsUpgradePrompt() && RestoreSavedUpgrades());
 const auto saved=ResolveUpgradeChoice();ResetUpgradeChoice();check(NeedsUpgradePrompt());
 ChooseUpgradeMode(saved==2);check(RestoreSavedUpgrades());
 defaultUpgradeReproduction=false;check(RestoreSavedUpgrades()); // another default cannot replace a saved answer
 ChooseUpgradeMode(false);check(!NeedsUpgradePrompt() && !RestoreSavedUpgrades());
 ResetUpgradeChoice();skipUpgradePrompt=true;defaultUpgradeReproduction=true;
 check(!NeedsUpgradePrompt() && RestoreSavedUpgrades());
 defaultUpgradeReproduction=false;check(RestoreSavedUpgrades()); // skipped answer is sealed for this save
 ResetUpgradeChoice();check(!NeedsUpgradePrompt() && !RestoreSavedUpgrades());
 skipUpgradePrompt=false;check(!NeedsUpgradePrompt()); // toggling skip does not reset an existing choice
 ResetUpgradeChoice();check(NeedsUpgradePrompt());check(!ValidUpgradeChoice(0) && !ValidUpgradeChoice(3));
 check(ValidUpgradeChoice(1) && ValidUpgradeChoice(2));
 Item original{"Armor.esp",0x800,"Armor","armor",""};original.smithing={1.5f,1234,12};original.enchantment="saved-recipe";original.preferred={true,1234,12,"owned-signature"};
 auto local=original;UseBasePropertiesIfMissing(local,false);
 check(local.smithing.health==1.f && local.enchantment.empty() && !local.preferred.custom);
 check(original.smithing.health==1.5f && original.enchantment=="saved-recipe" && original.preferred.custom);
 local=original;UseBasePropertiesIfMissing(local,true);
 check(local.smithing==original.smithing && local.enchantment==original.enchantment && local.preferred.custom);
}
