# Validation
Both normal and transmog UI requests now call prepareApplyPreset, which copies all three Library equipment-selection fields. Existing filter remains on game thread before staging. In-game validation pending.
- MSVC build passed; CTest 40/40 passed; PE metadata 1.0.17.7 verified. UI entry-point wiring reviewed; automated tests do not simulate game UI.
