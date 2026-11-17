# Missing-smithing notice localization

Base: test4. Metadata2.2.4.5. Only Localization.inc, TranslationKeys.h, Translation.template.json, StatusTranslationTest.cpp, version strings and documentation differ. Equipment.cpp, ManagedItems.cpp, Smithing.h, GeneratedIdentity.h, Presets.cpp/.h are byte-identical to test4.

Screenshot1 uses the new smithing-instance preflight failure, which intentionally differs from the old generic base-item ownership failure in screenshot2. Both correctly stop before generation when addMissing is false. Preserve that distinction; add the exact smithing error key to Japanese and external translation template. Do not modify ownership or generation behavior.

Status regression tests cover Japanese smithing and generic ownership notices, unchanged English, external override and external missing-key fallback. Runtime Japanese notice appearance remains for user verification.

User reports on 2026-11-17 confirm the planned functional tests: ordinary original possession preservation, both follower smithing qualities/other outfit exchange and reclaim, follower game-restart outfit retention, and missing-generation-OFF behavior. Prior reports confirm tail removal, reproduced quality, ID collision repair, enhanced/enchanted copy protection/reuse and save/restart/new-game quality reproduction. Treat as user runtime confirmation, not direct assistant log verification when no log was attached.
