# Preview pause controls trial

Base ui-test3, metadata 2.2.6.5. Extract the existing photo-pause button (including frozen/owned guard, tooltip and state colors) into RenderPhotoPauseButton. Toolbar still calls it. Small no-background child anchored 8 UI pixels inside preview image min hosts the same button; child hit area isolates it from pan. Applies to detached live preview and transparent camera region. Child and button disable under save/help/binding guards.

Pure PreviewPauseGesture arms only for hovered native double-click press, toggles on second release only if no drag and still hovered. Cancels with right button/disabled UI, rejects a double-click sourced from previous drag, resets on opening serial or preview-mode change and collapsed window. Camera drag/wheel code remains in place. Both actions request existing game-thread photoPause.Toggle; no freeze-time engine change, hooks, serialized setting or extra persistence. Existing PhotoPause ownership and release-on-close remain intact.

New gesture tests cover single click, second-release confirmation, no repeat, drag cancellation, preceding drag, outside release, right button, disabled UI and reset. Existing PhotoPause tests cover ownership. Native input/hit-testing, overlay layout and different runtime/SMF versions require in-game verification.
