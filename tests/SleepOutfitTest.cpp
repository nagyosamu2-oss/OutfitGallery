#include "AutoOutfitPolicy.h"
#include <cassert>
int main() {
    using namespace Gallery::AutoOutfit;
    Slots slots{};slots[0]="normal.png";slots[2]="home.png";
    Environment env{};env.sleeping=true;env.night=true;env.sneaking=true;
    // Empty assignment is inert; night and sitting alone are not sleep.
    assert(SelectCondition(Place::Home,env,false,slots)==Place::Home);
    for(unsigned state=0;state<16;++state) assert(SleepingState(state)==(state>=5 && state<=8));
    slots[27]="sleep.png";slots[22]="sneak.png";slots[5]="night.png";
    assert(SelectCondition(Place::Home,env,false,slots)==Place::Sleeping);
    assert(Choose(Place::Sleeping,slots,false)=="sleep.png");
    assert(SelectCondition(Place::Home,env,true,slots)==Place::Combat);
    Policy policy;
    assert(policy.Observe(static_cast<int>(Place::Home),1000,0));
    assert(policy.Attempt("home.png",1000));policy.Applied("home.png");
    assert(policy.Observe(static_cast<int>(Place::Sleeping),7000,0));
    assert(policy.Attempt("sleep.png",7000));policy.Applied("sleep.png");
    assert(!policy.Observe(static_cast<int>(Place::Sleeping),17000,0));
    // Wake re-evaluates current location, rather than restoring a snapshot.
    env.sleeping=false;env.sneaking=false;
    const auto wake=SelectCondition(Place::Home,env,false,slots);
    assert(wake==Place::Home);
    assert(policy.Observe(static_cast<int>(wake),18000,0));
    assert(policy.Attempt(Choose(wake,slots,false),18000));
    assert(ChooseSlot(Place::Sleeping,Slots{},false)==-1);
}
