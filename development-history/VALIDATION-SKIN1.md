# Trial validation — 2026-11-01

- Isolated source based on work/OutfitGallery-1.0.17; public source unchanged.
- MSVC 19.44 / CommonLibSSE-NG 10.1.0 bundle, RelWithDebInfo: build succeeded.
- 40 CTest cases passed. Six persistence tests initially failed under the filesystem sandbox; all six passed when rerun outside it.
- PE export inspection: OutfitGallery 1.0.17.1; existing runtime whitelist preserved. The new render hooks themselves install only on Steam 1.6.1170, after call-site validation.
- Software tests cover original/empty/shield slot ownership and hook-chain validation, alongside the existing 38 regression checks.
- Automated tests do not execute the game's renderer. Skin restoration, UBE, first person, clearing appearance, and performance remain in-game validation items.

Trial UI: context-menu action (command 30), explicit restore (command 31), session-only state. Ordinary player outfit application clears the look. Loading clears it without triggering a 3D update on the old actor.
The new render pass stages actor skin before appearance armor, then measures final surviving addons including skin for empty-slot ownership. It does not walk the inventory to calculate displaced coverage.
Existing real-geometry hiding is retained; custom body morph application is not added in this trial.
