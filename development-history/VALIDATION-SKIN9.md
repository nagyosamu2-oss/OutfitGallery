# Validation
Automatic appearance path branches before inventory snapshot/equip/verification. Existing timer and game-thread lane reused. No added hooks or polling. Session toggle marshalled through command 32. In-game validation pending.

Build: MSVC 14.44, RelWithDebInfo, successful.
CTest: 40/40 existing regression tests passed. These do not simulate Skyrim rendering or the new automatic transmog integration.
Runtime metadata: 1.0.17.9 verified. Appearance feature remains Steam 1.6.1170 only.
Automatic-owned state is atomic and keeps the existing scheduler eligible for cleanup after disabling automatic outfits.
In-game tests remain required: condition transitions, manual override, pause/disable, load/travel, normal mode and followers.
