#include "PadInputPolicy.h"
#include <cstdlib>
void check(bool v) {if(!v) std::abort();}
int main() {
    using namespace PadInputPolicy;
    // Raw event first, translated event next frame: exactly one UI action.
    check(SelectSource(8,0,8)==0);
    check(SelectSource(0,8,8)==8);
    check(SelectSource(8,8,8)==8);
    check(SelectSource(8,0,8)==0); // delayed raw cannot replay translated press
    check(SelectSource(32,0,8)==32); // unlisted custom bindings retain raw path
    check(SelectSource(8,0,8)==0); // translated release clears stale raw held bit
    RepeatState r;
    check(r.Update(1,1,1000)==1);
    check(r.Update(0,1,1200)==0);
    check(r.Update(0,1,1350)==1); // held navigation still repeats
    check(!MayReleaseFocus(0,false,false,false)); // repeat cannot steal text focus
    check(MayReleaseFocus(1,false,false,false)); // intentional fresh press works
    check(!MayReleaseFocus(1,true,false,false));
    check(!MayReleaseFocus(1,false,true,false));
    check(!MayReleaseFocus(1,false,false,true));
    r.Reset();
    check(r.Update(0,0,2000)==0); // reset plus cleared held mask
    check(r.Update(2,2,2010)==2);
    check(r.Update(0,2,2359)==0);
    check(r.Update(0,2,2360)==2);
}
