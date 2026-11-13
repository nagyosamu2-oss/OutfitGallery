# Trial 2 validation — 2026-11-01

- DLL version: 1.0.17.2. Separate trial source/build; public release untouched.
- MSVC 19.44 / CommonLibSSE-NG 10.1.0 build passed (existing AllocTrampoline deprecation warning retained).
- All 40 CTest cases passed outside the filesystem sandbox; exported PE metadata check passed.
- View eligibility test covers absent, first-person, third-person, unknown, and aliased holders. Only a distinct, recognized third-person holder is eligible.
- Source inspection: no mask hook installation; retained original visitor chaining; culling restricted to third person; original equipment/inventory operations not added.
- Added update start/completion and visitor-view diagnostics.
- In-game CTD resolution is NOT verified. This is a narrowed candidate fix, not proof of the exact root cause. Automatic tests do not execute Skyrim's renderer.
- Known tradeoff: first-person appearance remains real equipment, and original body/hair coverage can still hide geometry. Further work depends on the reproduced Outfit 174 -> Outfit 166 result.
