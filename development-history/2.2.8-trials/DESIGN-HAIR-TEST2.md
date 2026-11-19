# Reverse hair staging and FSMP default-hair isolation

Base: 2.2.7-hair-test1. Trial metadata 2.2.7.2.

User confirmed Painter Hair(real41) -> Yuna(appearance31), clear and reapply
work with test1. New reports reverse that direction and show a separate default
headpart physics issue. Screenshots20:20:22 /20:20:36 and the user clarify Yuna
is the real wig in the reverse comparison. The saved log also contains an HH wig
case; that ARMO additionally claims42 and has separate collision/visual addons,
so it is NOT treated as evidence of the exact same owner-coverage case.

## Reverse staging correction

The same engine branch measured in test1 applies in either direction. Yuna's
ARMO declares31 and its UBE ARMA covers31/41. Painter's ARMO declares41 and its
UBE ARMA covers31/41. Test1 only inspected object31, missing a marker on41.

Extend the existing scoped worn-owner binding to object41 as well. For each,
use its own requested bit, scope bit, modeled/race-compatible addon, exact
marker and unique worn owner checks. All original head-only31/41 ownership
limits, body exclusions and partial-look exclusions remain. This is not a
general arbitrary-slot normalization. No new calls/hooks, no direct addon/model
clearing, no priority changes, and no extra refresh or polling.

Tests add reverse41 admission, partial-look exclusion, wrong requested bit,
non-hair slot and out-of-range slot checks. The original lifecycle/rejection
tests remain. Runtime outcome still needs the user's Yuna->Painter comparison.

## Default headpart SMP issue: diagnosed separately, NOT fixed by this DLL

The user confirms native/default hair stays rigid after leaving the menus.
The installed FSMP4.1.1 winner is Faster HDT-SMP-4. Winner userConfigs.json in
MO2 overwrite has smp.disableSMPHairWhenWigEquipped=true.

FSMP upstream ActorManager.cpp (read2026-11-19) identifies hair armor via actual
ExtraWorn/ExtraWornLeft inventory entries on31/41. setHeadActiveIfNoHairArmor
sets skeleton->head.isActive=false when such armor exists. Its own source
explicitly describes disabling headpart physics behind a wig. Outfit Gallery
keeps actual equipment unchanged, so culling the real wig's model does not
remove that inventory predicate. This is consistent with the default mesh
being visible while its physics remains disabled.

Primary source: https://github.com/DaymareOn/hdtSMP64/blob/master/src/ActorManager.cpp
This is upstream source, not proof of the entire installed4.1.1 binary. The
setting's local true value is directly verified. Asked the user to compare
with the FSMP setting OFF; no FSMP configuration was modified by this task.
That option applies to FSMP globally; keeping hidden native-hair physics active
can add work. Do not present this as a local/per-actor compatibility fix.

No default-headpart rebuild, FSMP private-memory write, SMP reset command or
equipment toggle is added. A per-actor bridge needs separate API/ownership
validation and is not implemented speculatively.
