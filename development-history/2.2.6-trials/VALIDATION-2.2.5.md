# Validation — 2.2.5

Based on user-confirmed enchant-test6. Native source differs only in main.cpp version metadata, footer and log labels. Tests, translations, integrations, recipes, identity/cleanup behavior and default INI are unchanged.

User-reported in-game confirmation across trials: single/dual and copied multi-effect custom enchantments; smithing combinations; original item reuse; missing-item reproduction; alternating outfits without observed duplicates; generated-item cleanup; modified-item protection/reuse; save/restart and new-game reproduction; equipment-management enchanted marker. Final test6 reports: skip confirmation survives restart, reproduction OFF survives save/restart, OFF generates default properties, ON restores stored recipes, protected newly enchanted items are reused in OFF, transmog unaffected, follower effects active and generated items reclaimed.

The last OFF-owned-item test specifically covered a generated item enchanted afterward and retained, rather than independently proving every original-owned-item path. The original-owned path was confirmed earlier with reproduction ON. Nonzero area/duration, every external instance-replacement mod and every runtime are not established by these tests. Gold values may differ. Final numbered DLL needs a brief load/equip/cleanup smoke check; functional changes were not introduced during release packaging.

59 selected automated tests; known TrashVirtualPathSafety junction-fixture limitation excluded, not counted as passing. Build/export/archive results are recorded in BUILD-CHECKS-2.2.5.txt. Trial docs are preserved in development-history/2.2.5-trials. No install, GitHub sync, publishing or upload is performed by packaging.
