# Validation — skin13-diag2 (1.0.17.15)

2026-11-02. Independent copy of skin13-diag1; the original sources are retained.

Purpose: compare the actually displayed third-person addon/model after normal equipment with the candidates rejected by appearance preflight. Instrumentation runs on an appearance request, including rejected requests. It does not modify the normal-equipment implementation, load models, relax race/sex checks, change race, or enable player first-person styling.

Record findings (current MO2 profile: 9-1 OpenSh Test):

- Valmilia's actor base 000815:ValmiliaFollower.esp uses 013741:Skyrim.esm (BretonRace), matching runtime actor FE1E2823's trace.
- Black Cat Suit's sole addon 00081B:[swan] UBE BlackCat Follower.esp targets UBE races, with a female model. Treat as outside this Breton follower comparison after the user's clarification. No compatibility override is implemented.
- BDOR Cernay Armor resolves to BDOR Cernay UBE patch.esp and references 000818:BDOR Cernay UBE patch.esp plus 000844:BDOR Cernay.esp. Both addons have the third-person mesh in the male field, with the female field absent. The vanilla addon includes BretonRace; the UBE addon targets UBE races.
- Existing preflight requires both IsValidRace and a nonempty model in the actor's exact sex field. The female Cernay candidates are therefore rejected. Same-actor normal visual success is user-reported; the actual addon/model selected by the engine is the remaining observation needed. A fallback or another runtime integration is a hypothesis, not a confirmed cause.

Validation results are recorded in the delivered build-checks text. No in-game run was performed by the assistant. Software regression tests do not validate model rendering. This build deliberately retains the original rejection and is not a confirmed fix.

Source changes from diag1: diagnostic observations in Transmog.cpp, version 1.0.17.15 in main.cpp and the matching metadata assertion, plus this version's documentation. Equipment.cpp, FollowerOutfits.cpp, TransmogIdentity.h, tests other than the metadata assertion, CMake and all drawing decisions are unchanged.
