#pragma once

namespace Gallery {
enum class StudioMovement { Denied, Owned, External };
enum class StudioHeadtracking { Denied, Owned, External };

// An external disable owner already suppresses TDM headtracking.
// Leave that owner's resource alone and only release grants owned by Gallery.
template<class API, class Handle, class Result>
StudioHeadtracking RequestStudioHeadtracking(API& api, Handle handle, Handle invalid, Result& result) {
    result = api.RequestDisableHeadtracking(handle);
    if (result == Result::OK || result == Result::AlreadyGiven)
        return StudioHeadtracking::Owned;
    if (result == Result::AlreadyTaken) {
        const auto owner = api.GetDisableHeadtrackingOwner();
        if (owner != invalid && owner != handle)
            return StudioHeadtracking::External;
    }
    return StudioHeadtracking::Denied;
}

// Uses only public TDM queries. External ownership is never released or claimed.
// Fluid Motion's INI may differ from its live menu settings; query TDM instead.
template<class API, class Handle, class Result>
StudioMovement RequestStudioMovement(API& api, Handle handle, Handle invalid,
    bool fluidLoaded, Result& result) {
    result = api.RequestDisableDirectionalMovement(handle);
    if (result == Result::OK || result == Result::AlreadyGiven)
        return StudioMovement::Owned;
    if (fluidLoaded && result == Result::AlreadyTaken &&
        api.GetDisableDirectionalMovementOwner() != invalid &&
        api.GetDisableDirectionalMovementOwner() != handle &&
        !api.GetDirectionalMovementState())
        return StudioMovement::External;
    return StudioMovement::Denied;
}
}
