# Enchantment test5

Base: test4, metadata 2.2.5.14. Public 2.2.4 and prior archives retained.

Add per-save reproduction choice: 0 undecided, 1 base, 2 saved upgrades. SKSE co-save UPGR (0x55504752), version 1, length 1, uint8 value 1/2. Reset at revert/load. Missing/invalid/duplicate record leaves undecided. Global settings reproduceUpgrades (default true) and skipUpgradePrompt (default false) are defaults only. Skip resolves a fresh save once; later global changes do not override that save. Choice persists when the game is saved.

First gallery opening displays two localized buttons before other outfit UI. Gamepad up/down selects; A confirms. The answer frame is consumed. Gallery action dispatch waits for an answer. Automatic outfits before the first answer use basic generation. No new engine hooks or arbitrary form writes.

Options: reproduction checkbox followed by skip-confirmation on the same line. Reproduction updates the active save and global default. Skip is global. Existing save selections are independent. First-use text explains owned properties, later Options and game-save persistence.

ApplyEquipment snapshots policy. In OFF mode a local clothing-preset copy drops smithing/enchantment/preferred fields only for missing recipes with no known owned match. Original/associated generated or retained items remain usable with their properties. Generation cannot restore saved upgrades in OFF mode. Saved presets are not rewritten, so re-enabling restores recipes. Built-in base enchantments remain. Existing ledger identity, transfer handling and cleanup safeguards remain unchanged. Verification suppresses expected missing-upgrade warnings in basic mode.

Tests: policy default/prompt/skip/sealing/reset/valid values; local recipe stripping without source mutation; owned recipe preservation; global StudioSettings defaults and roundtrip. 59 selected tests pass; known environment fixture excluded. Native UI/lifecycle behavior awaits user runtime testing. test4 nonzero area/duration remains runtime-unverified.
