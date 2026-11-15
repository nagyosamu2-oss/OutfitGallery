#include "PhotoPause.h"
#include <cassert>
int main() {
 Gallery::PhotoPause p; bool frozen=false;
 p.Toggle(frozen); assert(frozen && p.Owns());
 p.Toggle(frozen); assert(!frozen && !p.Owns());
 p.Toggle(frozen); p.Release(frozen); assert(!frozen && !p.Owns());
 p.Release(frozen); assert(!frozen);
 frozen=true; p.Toggle(frozen); assert(frozen && !p.Owns());
 p.Release(frozen); assert(frozen && !p.Owns());
 frozen=false; p.Toggle(frozen); frozen=false; p.Release(frozen); assert(!frozen && !p.Owns());
}
