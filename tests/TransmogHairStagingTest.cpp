#include "TransmogHairStaging.h"
#include <cassert>
#include <stdexcept>
using namespace Gallery;
struct Form {bool marker;};
struct Object {Form* item;int* addon;int* part;};
int main() {
    constexpr auto hair=1u<<1,longhair=1u<<11,body=1u<<2;
    auto eligible=[&](auto request,auto scope,auto owner,auto coverage,
        bool marker=true,bool modeled=true,bool fits=true,bool style=false,bool unique=true) {
        return MayBindSecondaryHairOwner(1,request,scope,owner,coverage,marker,modeled,fits,style,unique);
    };
    assert(eligible(hair,~0u,longhair,hair|longhair));
    // Reverse direction: real31/Yuna leaves a secondary marker on41,
    // while the appearance Painter ARMO is declared on41.
    assert(MayBindSecondaryHairOwner(11,longhair,~0u,hair,hair|longhair,true,true,true,false,true));
    assert(!MayBindSecondaryHairOwner(11,longhair,longhair,hair,hair|longhair,true,true,true,false,true));
    assert(!MayBindSecondaryHairOwner(11,hair,~0u,hair,hair|longhair,true,true,true,false,true));
    assert(!MayBindSecondaryHairOwner(2,body,~0u,hair,hair|longhair,true,true,true,false,true));
    assert(!MayBindSecondaryHairOwner(32,~0u,~0u,hair,hair|longhair,true,true,true,false,true));
    assert(eligible(hair,hair|longhair,longhair,hair|longhair));
    assert(!eligible(body,~0u,longhair,hair|longhair));
    assert(!eligible(hair,hair,longhair,hair|longhair)); // untouched real item
    assert(!eligible(hair,~0u,body,hair|body));
    assert(!eligible(hair,~0u,longhair,hair|longhair|body)); // shared body model
    assert(!eligible(hair,~0u,longhair,hair|longhair,false));
    assert(!eligible(hair,~0u,longhair,hair|longhair,true,false));
    assert(!eligible(hair,~0u,longhair,hair|longhair,true,true,false));
    assert(!eligible(hair,~0u,longhair,hair|longhair,true,true,true,true));
    assert(!eligible(hair,~0u,longhair,hair|longhair,true,true,true,false,false));
    Form marker{true},worn{false},look{false};int oldAddon{},oldModel{},newAddon{},newModel{};
    Object obj{&marker,&oldAddon,&oldModel};
    // Model the observed engine branch: a tied ARMA occupant is retained;
    // an ARMO occupant goes through the engine's detach/replacement path.
    auto stage=[&] {
        if(obj.item && obj.item->marker) return;
        obj={&look,&newAddon,&newModel};
    };
    stage();assert(obj.item==&marker && obj.addon==&oldAddon);
    {
        SecondaryHairOwnerBinding binding(obj,&worn);
        stage();
    }
    assert(obj.item==&look && obj.addon==&newAddon && obj.part==&newModel);
    obj={&marker,&oldAddon,&oldModel};
    {SecondaryHairOwnerBinding binding(obj,&worn);} // rejected/no-op staging
    assert(obj.item==&marker && obj.addon==&oldAddon && obj.part==&oldModel);
    try {
        SecondaryHairOwnerBinding binding(obj,&worn);
        throw std::runtime_error("staging stopped");
    } catch(const std::runtime_error&) {}
    assert(obj.item==&marker);
    {SecondaryHairOwnerBinding binding(obj,static_cast<Form*>(nullptr));stage();}
    assert(obj.item==&marker);
    {
        SecondaryHairOwnerBinding binding(obj,&worn);
        obj={nullptr,nullptr,nullptr}; // engine detached; never resurrect stale metadata
    }
    assert(!obj.item && !obj.addon && !obj.part);
}
