# Smithing reproduction trial

Base: confirmed 2.2.3-ui-background-test1, metadata 2.2.3.4. Trial metadata 2.2.4.1.

Portable preset metadata is one versioned `smithing` object: float health and optional original-instance scope/id. No runtime form/pointer or dynamic enchantment is exported. Legacy presets are unchanged unless re-registered. Unknown or invalid recipe versions/values fail explicitly, rather than quietly dropping quality. Original identities remain scoped to the SKSE co-save; recipe health does not depend on that scope.

Selection uses the original scoped singleton, else a tracked or retained reproduction of the same recipe, else generation (if enabled). Equivalent foreign possessions are never adopted or rewritten. Candidate ambiguity stops the operation. Existing inventory refresh/engine equip and verification remain in the game-thread transaction. Added filtered inventory queries are conditional on a valid smithing recipe. No new polling or hooks.

New lists are allocated with the game heap at runtime size 0x18 before AE1.6.629 and 0x20 after it. CommonLib's engine constructor/destructor IDs are taken directly from alandtse/CommonLibVR ng/src/RE/E/ExtraDataList.cpp (11437/11583 and 11438/11584). This workspace's trimmed CommonLib bundle omits the exported ctor/dtor wrappers, so those same engine functions are called directly. Never allocate using compile-time sizeof(ExtraDataList). Ownership of a successfully submitted list goes to the engine. Game-runtime allocation, transfer notifications and equip behavior still require empirical verification.

Only freshly generated items receive ExtraHealth. Claim and expected health precede AddObjectToContainer, allowing outgoing transfers to cancel ownership. Reacquire fresh scalar-identified singleton after generation and again after unequipping. Never fall back to a base-form deletion or modify an existing plain item.

New PERF co-save records (v1,15 bytes each) store form/owner/UID + expected health + reproduction/retained flags. ITEM records remain unchanged for older readers. SMID (v1,14 bytes) scopes follower original identities. IDs are resolved on load. Duplicate or malformed performance records cannot authorize less restrictive deletion. Legacy ITEM claims without baseline retain prior protection. Co-save baselines are not copied into global JSON and cannot cross new-game boundaries.

Known current identities use performance deltas instead of name/existence-only protection. Later stronger smithing and any custom enchantment preserve the object. Preserved reproduction identities are retained only for reuse, never deletion. Quest/favorite/poison/worn safeguards remain. Transfers relinquish recipe associations; UID changes invalidate baseline evidence. Co-save migration never infers a new claim from an arbitrary existing possession.

Tests: portable JSON and exact float roundtrip, legacy absence, invalid/unknown recipes, original/reproduction/foreign selection, ambiguity and stacks, performance deltas and runtime allocation sizes. Builds cannot establish load/equip safety, timings, cleanup notifications or graphical results; use the dedicated test save procedure.

Source reference: https://github.com/alandtse/CommonLibVR/blob/ng/src/RE/E/ExtraDataList.cpp

Final audit: the container event from our own singleton RemoveItem must not relinquish every remaining claim of the same base form. A mutex-protected scoped identity filters only that call's matching deletion notification (including the engine's absent UID case). It does not suppress manual transfers, different IDs/counts/destinations. Existing verified count/identity checks after removal remain. Pure policy tests cover all of those boundaries; runtime two-quality alternation is explicitly in the test guide.
