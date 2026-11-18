# Enchantment test4

Base test3. Display: ResolvePresetDisplayNames optionally outputs a parallel enchanted bit vector based on saved recipe, legacy preferred.custom or armor.formEnchanting. The existing game-thread scope request prepares it and publishes it under presetsMutex with the same photo guard; UI copies it and appends a localized [Enchanted] label. Clear with names on request. No record mutation, name/identity persistence change, or new equip-time scan. Built-in enchantment flags are read on the game thread, not from UI.

Relax only zero-area/zero-duration trial restriction: the format already stores both uint32 fields and the native factory input already copies them. Validate full uint32 bounds instead of zero. Negative/overflow rejected; factory and baseline comparisons still include both fields. Constant/self armor and no per-effect-condition restrictions remain. Magnitude/cost/health trial numeric bounds, static references, 32-effect and file-size limits remain.

Tests cover nonzero and UINT32_MAX field roundtrip, negative/overflow rejection, changed area/duration mismatch and actual preset persistence (area25,duration60). Existing 1/2/32-effect and cleanup comparison tests retained. Nonzero area/duration engine behavior and new label layout need runtime confirmation.

Rename audit: Encode saves base names; EnchantmentSignature excludes display-name data. Known generated performance baselines ignore name-only cleanup protection, while quest/favorite/poison/actual enchantment/smithing safeguards remain. No private item-name preservation is newly added. External systems that replace items or change identity are outside this name-only observation; compatibility not guaranteed.

Previous user confirmations: test3 all-effects registration, new-game reproduction, multiple outfit cycles, cleanup and no observed accumulation. test2 restart cleanup and retained modified-item reuse reported OK. These reports are separate from new display/nonzero-field validation.
