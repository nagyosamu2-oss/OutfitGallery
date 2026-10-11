#include "StudioSafety.h"
#include <iostream>
int main() {
 using Gallery::StudioCombatAllowed;
 if(!StudioCombatAllowed(false,false,true,false)) return 1; // ordinary player entry remains available
 if(StudioCombatAllowed(true,true,true,false)) return 2; // opt-out blocks player combat
 if(!StudioCombatAllowed(true,true,true,true)) return 3; // opt-in permits player combat
 if(!StudioCombatAllowed(false,false,false,true)) return 4; // peaceful follower still available
 if(StudioCombatAllowed(true,false,false,true)) return 5; // player combat blocks peaceful follower
 if(StudioCombatAllowed(false,true,false,true)) return 6; // follower combat blocks peaceful player
 if(StudioCombatAllowed(true,true,false,true)) return 7; // both fighting never bypasses follower gate
 // Idle, furniture and scene activity are not hazards. Each actual hazard blocks.
 Gallery::FollowerHazards safe{};
 if(Gallery::FollowerStateReason(safe)) return 8;
 bool Gallery::FollowerHazards::* flags[]{&Gallery::FollowerHazards::unavailable,&Gallery::FollowerHazards::dead,
 &Gallery::FollowerHazards::killMove,&Gallery::FollowerHazards::ragdoll,&Gallery::FollowerHazards::unconscious,
 &Gallery::FollowerHazards::mounted,&Gallery::FollowerHazards::swimming,&Gallery::FollowerHazards::airborne};
 for(auto flag:flags) {auto state=safe; state.*flag=true; if(!Gallery::FollowerStateReason(state)) return 9;}
 std::cout<<"PASS: opt-in player combat entry and strict follower combat gates\n";
}
