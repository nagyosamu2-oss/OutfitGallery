# enchant-test3 — bounded multi-effect expansion

Base enchant-test2. The screenshot reached "Probe supports one or two effects", i.e. the portable schema's initial 2-effect limit. The prior EnchantmentSignature capture already has a 32-effect ceiling and returned data before this error. Align recipe validation and the native factory Encode guard to MaxEffects=32. The actual factory already accepts a BSTArray<Effect> and loops all saved effects; no new native engine API or memory layout is required.

Do not raise the existing 16384-byte signature, 256KiB preset, bounded EPRF storage or numeric/reference limits. Keep constant/self/zero area/duration/no conditions and exact non-price metadata validation. One unsupported effect rejects the whole recipe; no truncation. 1/2-effect data formats remain identical. Preserve price normalization and health/modified-item protection from test2. No new per-frame work or inventory scans.

Tests: 3 and32 effect recipe roundtrips, 33 rejection, NaN in effect32 rejection, last-effect magnitude mutation detection, order mismatch rejection, and 32-effect preset persistence without truncation. Existing tests also run. This proves bounded data handling only; engine creation, all active effects, cleanup, persistence and timing for the user's copied all-effects item require runtime checks.

Native changes are only one factory output count guard plus version labels. ManagedItems, Equipment, Presets serialization, factory input loop and existing EnchantmentSignature source remain byte-identical to test2.
