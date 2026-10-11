#include "FormIdentityPolicy.h"
#include <cassert>
#include <stdexcept>
struct File {unsigned compileIndex,masterCount{};const File** masterPtrs{};};
struct Form {const File* origin;};
struct Handler {
 File full{0x6D},light{0xFE};
 const File* LookupLoadedModByIndex(unsigned i){return i==0x6D?&full:nullptr;}
 const File* LookupLoadedLightModByIndex(unsigned i){return i==0xABC?&light:nullptr;}
};
void check(bool b){if(!b)throw std::runtime_error("identity regression");}
int main(){
 using namespace Gallery;Handler h;
 check(FormOwner(&h,0x6D000801)==&h.full);
 check(LocalFormID(0x6D000801)==0x801);
 check(FormOwner(&h,0xFEABC801)==&h.light);
 check(LocalFormID(0xFEABC801)==0x801);
 check(!FormOwner(&h,0xFF000801));
 check(!FormOwner(static_cast<Handler*>(nullptr),0x6D000801));
 File other{0x70};const File* masters[]{&h.full,&other};File patch{0xFE,2,masters};
 Form injected{&patch},unrelated{&other},direct{&patch};
 unsigned lookups=0;
 auto lookup=[&](const File* f,unsigned id)->Form*{++lookups;return id==0x801?(f==&h.full?&injected:&unrelated):nullptr;};
 auto match=[](Form* f,const File* p){return f->origin==p;};
 check(ResolveLegacyInjected(&direct,&patch,0x801,lookup,match)==&direct && lookups==0);
 check(ResolveLegacyInjected<Form>(nullptr,&patch,0x801,lookup,match)==&injected && lookups==2);
 check(!ResolveLegacyInjected<Form>(nullptr,&patch,0x802,lookup,match));
 unrelated.origin=&patch;
 check(!ResolveLegacyInjected<Form>(nullptr,&patch,0x801,lookup,match));
 patch.compileIndex=0xFF;
 check(!ResolveLegacyInjected<Form>(nullptr,&patch,0x801,lookup,match));
}
