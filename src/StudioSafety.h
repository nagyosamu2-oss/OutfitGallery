#pragma once
namespace Gallery {
// Combat entry is an opt-in for the player only. Followers retain both gates.
inline bool StudioCombatAllowed(bool playerCombat,bool targetCombat,bool playerTarget,bool allow) {
    return (!playerCombat && !targetCombat) || (playerTarget && allow);
}
// Furniture use, sitting, ordinary idles and scenes alone do not block entry.
struct FollowerHazards {
 bool unavailable{}, dead{}, killMove{}, ragdoll{}, unconscious{}, mounted{}, swimming{}, airborne{};
};
inline const char* FollowerStateReason(const FollowerHazards& s) {
 if(s.unavailable) return "Follower is not loaded or is disabled.";
 if(s.dead) return "Follower is dead.";
 if(s.killMove) return "Follower is in a killmove.";
 if(s.ragdoll) return "Follower is ragdolled.";
 if(s.unconscious) return "Follower is unconscious.";
 if(s.mounted) return "Follower is mounted.";
 if(s.swimming) return "Follower is swimming.";
 if(s.airborne) return "Follower is airborne (jumping or falling).";
 return nullptr;
}
}
