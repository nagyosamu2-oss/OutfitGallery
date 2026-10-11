#include "AutoOutfitPolicy.h"
#include <cstdlib>
void check(bool v) {if(!v) std::abort();}
int main(){using namespace Gallery::AutoOutfit;
 Policy p;p.lastAttempt=10000;
 check(!p.Observe(static_cast<int>(Place::Melee),11999,0,2000));
 check(p.Observe(static_cast<int>(Place::Melee),12000,0,2000));
 p=Policy{};p.lastAttempt=10000;
 check(!p.Observe(static_cast<int>(Place::Magic),14999,0));
 check(p.Observe(static_cast<int>(Place::Magic),15000,0));
 p=Policy{};p.lastAttempt=10000;
 check(!p.Observe(static_cast<int>(Place::Town),12000,0,2000));
 check(p.Observe(static_cast<int>(Place::Town),15000,0,2000));
 p=Policy{};check(p.Observe(static_cast<int>(Place::Melee),100,0,2000));
 p=Policy{};p.lastAttempt=10000;
 check(!p.Observe(static_cast<int>(Place::Melee),12000,3000,2000));
 check(!p.Observe(static_cast<int>(Place::Melee),14999,3000,2000));
 check(p.Observe(static_cast<int>(Place::Melee),15000,3000,2000));
}
