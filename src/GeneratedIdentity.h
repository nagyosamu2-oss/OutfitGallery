#pragma once
#include <cstdint>
namespace Gallery {
// Exists only while our synchronous AddObjectToContainer call is active.
struct FreshAdditionObservation {
    std::uint32_t actor{},form{};
    bool cancelled{};
    std::uint16_t Observe(std::uint32_t object,std::uint32_t from,std::uint32_t to,int count,std::uint16_t uid,bool pendingSingleton) {
        if(!actor || !form || object!=form) return 0;
        if(from==actor && from!=to) cancelled=true;
        return !cancelled && !from && to==actor && count==1 && uid && pendingSingleton?uid:0;
    }
};
struct GeneratedIdentityEvidence {
    bool newPending{}, observedAfterCreation{};
    std::uint32_t actor{}, observedOwner{}, currentOwner{};
    std::uint16_t observedID{}, currentID{};
    int expectedCount{}, currentCount{}, instanceCount{};
    unsigned matchingIDs{};
    bool reserved{};
};
inline bool CanAdoptGeneratedIdentity(const GeneratedIdentityEvidence& e,unsigned formLists=1) {
    return formLists==1 && e.newPending && e.observedAfterCreation && e.actor &&
        e.observedOwner==e.actor && e.currentOwner==e.actor &&
        e.observedID && e.currentID==e.observedID &&
        e.expectedCount==1 && e.currentCount==1 && e.instanceCount==1 &&
        e.matchingIDs==1 && !e.reserved;
}
}
