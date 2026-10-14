#include "FollowerAutoState.h"
#include "AutoOutfitPolicy.h"
#include <iostream>
using namespace Gallery;
void require(bool ok) {if(!ok) throw std::runtime_error("Follower automatic regression");}
int main() {
 try {
    FollowerAuto::ArrivalCheck arrival;
    arrival.Observe(10,true,false);
    require(!arrival.Ready(1000));require(arrival.Ready(3000));arrival.Checked();
    require(!arrival.Ready(4000));require(arrival.Ready(6000));arrival.Checked();
    require(!arrival.Ready(20000)); // cannot keep enforcing an unchanged cell
    arrival.Observe(11,true,false);require(!arrival.Ready(21000));require(arrival.Ready(23000));
    arrival.Cancel();require(!arrival.Ready(30000)); // repair or manual outfit consumes episode
    arrival.Observe(11,true,false);require(!arrival.Ready(31000));
    arrival.Observe(11,false,false);arrival.Observe(11,true,false);
    require(!arrival.Ready(32000));arrival.Block();require(!arrival.Ready(34000));require(arrival.Ready(36000));
    arrival.Cancel();arrival.Observe(11,true,true);arrival.Observe(11,true,false);
    require(!arrival.Ready(40000));require(arrival.Ready(42000));
    FollowerAuto::ArrivalCheck fast;
    fast.Observe(10,true,false);
    require(!fast.EarlyReady(true,1,1,"town.png","town.png")); // no early repair on registration
    fast.Observe(11,true,false);
    require(fast.EarlyReady(true,1,1,"town.png","town.png"));
    require(!fast.EarlyReady(true,1,0,"town.png","town.png"));
    require(!fast.EarlyReady(true,1,1,"town.png","other.png"));
    require(!fast.EarlyReady(false,1,1,"town.png","town.png"));
    fast.EarlyChecked();require(!fast.EarlyReady(true,1,1,"town.png","town.png"));
    require(!fast.Ready(1000));require(fast.Ready(3000)); // fallback remains armed
    fast.Checked();require(fast.Ready(6000));fast.Cancel();
    require(!fast.Ready(10000));
    fast.Observe(11,true,true);fast.Observe(11,true,false);
    require(fast.EarlyReady(true,1,1,"town.png","town.png"));
    fast.Cancel();require(!fast.EarlyReady(true,1,1,"town.png","town.png"));
    AutoOutfitSettings a,b;
    a.slots[0]="first.png";a.slots[4]="battle.png";a.enabled=true;a.combatEnabled=true;a.delaySeconds=0;a.combatIntervalSeconds=2;
    b.slots[0]="second.png";b.slots[1]="town.png";b.paused=true;b.delaySeconds=9;b.hideAssignButtons=true;
    auto ar=FollowerAuto::Decode(FollowerAuto::Encode(a)),br=FollowerAuto::Decode(FollowerAuto::Encode(b));
    require(ar.slots==a.slots && ar.enabled && ar.combatEnabled && ar.delaySeconds==0 && ar.combatIntervalSeconds==2);
    require(br.slots==b.slots && br.paused && br.delaySeconds==9 && br.hideAssignButtons && !br.enabled);
    auto invalid=[&](nlohmann::json j) {try {(void)FollowerAuto::Decode(j.dump());return false;}catch(...) {return true;}};
    const auto j=nlohmann::json::parse(FollowerAuto::Encode(a));
    auto bad=j;bad["slots"][0]="../other.png";require(invalid(bad));
    bad=j;bad["slots"]=nlohmann::json::array();require(invalid(bad));
    bad=j;bad["delay"]=1.5;require(invalid(bad));
    bad=j;bad["interval"]=1;require(invalid(bad));
    bad=j;bad["enabled"]=1;require(invalid(bad));
    bad=j;bad["slots"][23]="craft.png";require(invalid(bad));
    require(FollowerAuto::CanRegister(1,false));require(!FollowerAuto::CanRegister(2,false));require(FollowerAuto::CanRegister(2,true));
    AutoOutfit::Policy player,first,second;
    first.Manual(0);require(!first.Observe(0,1000,0));
    require(second.Observe(0,1000,0));require(player.Observe(0,1000,0));
    require(second.Attempt(br.slots[0],1000));second.Applied(br.slots[0]);
    require(player.lastPhoto.empty() && first.lastPhoto.empty());
    AutoOutfit::CombatSession ca,cb;ca.Manual(first);cb.Observe(true,second);
    require(!ca.Allowed(true,true));require(cb.Allowed(true,true));
    // Two ready actors: verification + shared spacing serialize equipment work.
    require(FollowerAuto::MayApply(1000,0,false));
    require(!FollowerAuto::MayApply(1000,2000,true));
    require(!FollowerAuto::MayApply(1750,2000,false));
    require(FollowerAuto::MayApply(2000,2000,false));
    // Offset polling clocks must not allow a ready follower to pass a ready
    // player just because the player's ordinary polling time is later.
    require(!FollowerAuto::PollPlayer(2000,2500,false));
    require(FollowerAuto::PollPlayer(2000,2500,true));
    require(FollowerAuto::PollPlayer(2500,2500,false));
    AutoOutfit::Policy waitingPlayer,waitingFollower;
    require(!waitingPlayer.Observe(1,1000,2000));
    require(!waitingPlayer.Observe(1,2000,2000));
    require(waitingPlayer.Observe(1,3000,2000));
    require(!FollowerAuto::MayApply(3000,4000,true));
    require(!waitingPlayer.handled && waitingPlayer.lastAttempt==0);
    // Verification ends: already elapsed stability is retained, not restarted.
    require(waitingPlayer.Observe(1,4000,2000));
    require(waitingFollower.Observe(1,4000,0));
    std::uint64_t lane=4000;
    require(FollowerAuto::MayApply(4000,lane,false));
    require(waitingPlayer.Attempt("player.png",4000));lane=5000;
    require(!FollowerAuto::MayApply(4000,lane,true));
    require(!waitingFollower.handled && waitingFollower.lastAttempt==0);
    require(waitingFollower.Observe(1,5000,0));
    require(FollowerAuto::MayApply(5000,lane,false));
    // A real condition change still restarts its delay; priority cannot bypass it.
    AutoOutfit::Policy changing;
    require(!changing.Observe(0,1000,2000));
    require(!changing.Observe(1,2500,2000));
    require(!changing.Observe(1,4000,2000));
    require(changing.Observe(1,4500,2000));
    // Keep the existing five-second noncombat interval after a combat attempt.
    AutoOutfit::Policy returning;
    require(returning.Observe(4,1000,0,2000));
    require(returning.Attempt("battle.png",1000));returning.Applied("battle.png");
    require(!returning.Observe(0,3000,0,2000));
    require(returning.Observe(0,6000,0,2000));
    require(AutoOutfit::Choose(AutoOutfit::Place::Town,br.slots,true)=="town.png");
    require(AutoOutfit::Choose(AutoOutfit::Place::Town,ar.slots,true)=="first.png");
    std::cout<<"Follower isolation, persistence, limits, validation and serialized apply passed\n";
 }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
