# Secondary hair marker staging

Base: public 2.2.7. Trial metadata 2.2.7.1. No new engine calls or hooks.

## Evidence

The user's controlled 19:49:10/19:49:30 log comparison keeps only Painter Hair
equipped versus removes it; both apply Outfit 174 with Yuna. In the failing run,
slot31 holds Painter ARMA FE36F80D, slot41 holds its worn ARMO FE2D5A9A, and Yuna
never survives staging. The real Painter clone is subsequently hidden. Removing
the real hair allows Yuna to stage in31/41 and attach.

Live 1.6.1170 read-only code inspection confirms the reason for this pattern:
stage17792 at RVA279320 walks race-valid addons and calls17759 at RVA2775D0.
That delegates to the biped updater at RVA213210. In that updater:

- RVA213324 distinguishes worn ARMO (type1A) from secondary ARMA (type66).
- For an ARMO in the incoming armor's declared slot, it follows the engine's
  own shared-model cleanup at21338A..2133F3, calling2132730 for matching parts.
- A surviving secondary ARMA instead reaches priority comparisons; the equal
  priority/no texture-swap-list case at213499..2134BA skips the incoming model.
- Painter/Yuna UBE addons both declare31/41 and female priority10. Painter's
  ARMO declares only41, which accounts for the secondary marker in31.

The previous helmet fix restores head/ear partition visibility. It cannot
attach a wig which did not stage, so no partition restoration is widened here.

## Narrow correction

Before a third-person appearance armor requesting31 is staged, inspect object31.
Require a modeled ARMA marker whose item equals its addon, a race/sex-compatible
incoming hair addon, and exactly one worn ARMO owner identified from the existing
per-pass realItems snapshot. The owner must have only hair31/41 declared/valid
coverage and its declared slot must be controlled by this look. Decline shared
ambiguous owners and already-selected appearance addons.

Temporarily lend object31 its positively identified worn ARMO owner while the
existing stageArmor call executes. The engine now takes its normal ARMO cleanup
path, detaching shared parts and replacing them. We never clear addon/part/clone
or smart-pointer fields ourselves, nor alter shared ARMA records, priorities,
inventory entries, selected look data, or actual equipment. No per-frame lookup
or added refresh is introduced. Existing final-addon measurement and per-slot
real identity restoration remain in place.

A scope guard restores the exact secondary item pointer if the engine leaves
item/addon/part untouched. If the engine detached or replaced those fields, it
keeps the engine's result and never resurrects stale model data. First-person
appearance staging stays on its existing path.

## Validation limits

Pure tests exercise guard exclusions, the observed tied-marker rejection model,
successful replacement, rejection/no-op rollback, exception cleanup, disabled
binding, and preservation of engine detachment. They do not execute Skyrim's
staging function. Live code is inspected on1.6.1170 only; visual outcome, SMP,
followers and other supported runtimes need in-game validation. Public2.2.7,
installed files and GitHub remain unchanged.
