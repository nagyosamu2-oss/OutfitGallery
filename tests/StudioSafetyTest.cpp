#include "StudioSafety.h"
#include <iostream>
int main() {
 using Gallery::StudioCombatAllowed;
 for(bool playerCombat : {false,true}) for(bool targetCombat : {false,true}) {
  if(!StudioCombatAllowed(playerCombat,targetCombat,true)) return 1;
  if(StudioCombatAllowed(playerCombat,targetCombat,false)!=(!playerCombat && !targetCombat)) return 2;
 }
 // Idle, furniture and scene activity are not hazards. Each actual hazard blocks.
 Gallery::FollowerHazards safe{};
 if(Gallery::FollowerStateReason(safe)) return 8;
 bool Gallery::FollowerHazards::* flags[]{&Gallery::FollowerHazards::unavailable,&Gallery::FollowerHazards::dead,
 &Gallery::FollowerHazards::killMove,&Gallery::FollowerHazards::ragdoll,&Gallery::FollowerHazards::unconscious,
 &Gallery::FollowerHazards::mounted,&Gallery::FollowerHazards::swimming,&Gallery::FollowerHazards::airborne};
 for(auto flag:flags) {auto state=safe; state.*flag=true; if(!Gallery::FollowerStateReason(state)) return 9;}
 std::cout<<"PASS: shared combat permission and unchanged follower hazards\n";
}
