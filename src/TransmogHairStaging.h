#pragma once
#include <cstdint>
namespace Gallery {
inline constexpr std::uint32_t HairStagingSlots=(1u<<1)|(1u<<11); // 31/41
// Only a positively owned secondary hair marker may borrow its worn ARMO.
// Leave real ARMO entries, skin, body pieces, partial-look exclusions and
// already staged appearance addons on their existing engine path.
constexpr bool MayBindSecondaryHairOwner(unsigned slot,std::uint32_t requested,std::uint32_t scope,
    std::uint32_t ownerSlots,std::uint32_t ownerCoverage,bool marker,bool modeled,
    bool incomingFits,bool alreadyAppearance,bool uniqueOwner) {
    if(slot!=1 && slot!=11) return false;
    return (requested&(1u<<slot)) && (scope&(1u<<slot)) &&
        (ownerSlots&scope) && (ownerSlots&HairStagingSlots) &&
        !(ownerSlots&~HairStagingSlots) && !(ownerCoverage&~HairStagingSlots) &&
        marker && modeled && incomingFits && !alreadyAppearance && uniqueOwner;
}
// Borrow only the plain item pointer for the synchronous staging call. The
// engine owns detachment and every addon/model/clone/refcount update. If it
// declined the call without changing the object, put its exact marker back.
template<class Object,class Form>
class SecondaryHairOwnerBinding {
    Object& object;
    Form* previous;
    Form* owner;
    decltype(Object::addon) addon;
    decltype(Object::part) part;
public:
    SecondaryHairOwnerBinding(Object& value,Form* wornOwner):
        object(value),previous(value.item),owner(wornOwner),addon(value.addon),part(value.part) {
        if(owner) object.item=owner;
    }
    ~SecondaryHairOwnerBinding() {
        if(owner && object.item==owner && object.addon==addon && object.part==part)
            object.item=previous;
    }
    SecondaryHairOwnerBinding(const SecondaryHairOwnerBinding&)=delete;
    SecondaryHairOwnerBinding& operator=(const SecondaryHairOwnerBinding&)=delete;
};
}
