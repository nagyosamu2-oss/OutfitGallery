#include "PreviewPauseGesture.h"
#include <cassert>
int main(){Gallery::PreviewPauseGesture g;
 auto step=[&](bool press,bool dbl,bool drag,bool release,bool hover=true,bool right=false,bool enabled=true){return g.Update(enabled,hover,press,dbl,drag,release,right);};
 assert(!step(true,false,false,false));assert(!step(false,false,false,true)); // single click
 assert(!step(true,true,false,false));assert(step(false,false,false,true)); // second release only
 assert(!step(false,false,false,true)); // never repeat
 assert(!step(true,true,false,false));assert(!step(false,false,true,false));assert(!step(false,false,false,true)); // drag cancels
 assert(!step(true,true,false,false));assert(!step(false,false,false,true)); // previous drag cannot be double-click source
 assert(!step(true,true,false,false));assert(!step(false,false,false,true,false)); // outside release
 g.Reset();assert(!step(true,true,false,false));assert(!step(false,false,false,true,true,true)); // right button cancels
 assert(!step(true,true,false,false));assert(!step(false,false,false,true,true,false,false)); // UI disabled
 assert(!step(false,false,false,true));
 assert(!step(true,true,false,false));g.Reset();assert(!step(false,false,false,true)); // reopen/mode switch
}
