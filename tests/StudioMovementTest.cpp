#include "StudioMovement.h"
#include <cassert>
#include <initializer_list>
enum class Result { OK, NotOwner, MustKeep, AlreadyGiven, AlreadyTaken, BadThread };
struct API {
    Result response;
    unsigned owner = 8;
    bool directional = false;
    unsigned requests = 0;
    Result RequestDisableDirectionalMovement(unsigned) { ++requests; return response; }
    Result RequestDisableHeadtracking(unsigned) { ++requests; return response; }
    unsigned GetDisableHeadtrackingOwner() { return owner; }
    unsigned GetDisableDirectionalMovementOwner() { return owner; }
    bool GetDirectionalMovementState() { return directional; }
};
int main() {
    using Gallery::StudioMovement;
    constexpr unsigned me=3, invalid=~0u;
    for(auto result : {Result::OK, Result::NotOwner, Result::MustKeep,
        Result::AlreadyGiven, Result::AlreadyTaken, Result::BadThread}) {
        for(unsigned owner : {me, 8u, invalid}) {
            API api{result,owner}; Result actual{};
            const auto control=Gallery::RequestStudioHeadtracking(api,me,invalid,actual);
            assert(actual==result && api.requests==1);
            if(result==Result::OK || result==Result::AlreadyGiven)
                assert(control==Gallery::StudioHeadtracking::Owned);
            else if(result==Result::AlreadyTaken && owner==8)
                assert(control==Gallery::StudioHeadtracking::External);
            else assert(control==Gallery::StudioHeadtracking::Denied);
            assert(api.owner==owner);
        }
    }
    // First-person camera ownership can end during the switch to free camera.
    API transition{Result::AlreadyTaken,8}; Result actual{};
    assert(Gallery::RequestStudioHeadtracking(transition,me,invalid,actual)==Gallery::StudioHeadtracking::External);
    transition.response=Result::OK; transition.owner=invalid;
    assert(Gallery::RequestStudioHeadtracking(transition,me,invalid,actual)==Gallery::StudioHeadtracking::Owned);
    for (bool fluid : {false,true}) {
        for (auto result : {Result::OK, Result::NotOwner, Result::MustKeep,
             Result::AlreadyGiven, Result::AlreadyTaken, Result::BadThread}) {
            for (unsigned owner : {me, 8u, invalid}) for (bool enabled : {false,true}) {
                API api{result,owner,enabled}; Result actual{};
                const auto decision=Gallery::RequestStudioMovement(api,me,invalid,fluid,actual);
                assert(actual==result && api.requests==1);
                if(result==Result::OK || result==Result::AlreadyGiven)
                    assert(decision==StudioMovement::Owned);
                else if(fluid && result==Result::AlreadyTaken && owner==8 && !enabled)
                    assert(decision==StudioMovement::External);
                else assert(decision==StudioMovement::Denied);
                // No ownership mutation: a caller must only release Owned results.
                assert(api.owner==owner && api.directional==enabled);
            }
        }
    }
}
