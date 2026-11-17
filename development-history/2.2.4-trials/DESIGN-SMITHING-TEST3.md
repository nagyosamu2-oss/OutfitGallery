# Reserved-ID repair for a proved new reproduction

Observed in user's 2026-11-17 11:15 log: first add form000A6D7F health1.3000001 uid1 before0/after1 verified. Second add health1.1 allocated4 received a synchronous same-owner UID event4->3; candidate3 was new, count1 and health1.1, and total1->2. Sole failed predicate was reserved=true. Its reservation source cannot be determined from test2's used-set union alone; do not attribute it to an external mod.

The engine considers live inventory IDs when selecting its next ID; gallery allocation additionally excludes prior saved reservations and raw/tracked IDs. Retag only this proved-new singleton back to the preallocated ID if that ID remains free across raw actor inventory lists (including zero-count entries). Do not relax the final reserved-ID check and do not invalidate original scopes to make room. This prevents later original-instance selection from confusing the new copy with a reserved original.

The repair is narrowly conditional: all preexisting proof predicates except reserved must pass; requested allocated ID must be nonzero/different, not in the pre-call used set, and not currently attached to any other owner-local raw list. Only that new ExtraUniqueID.uniqueID is edited, never base armor or a preexisting list. TESUniqueIDChangeEvent is sent through ScriptEventSourceHolder with old/new owner equal to the actor and old/new IDs matching the actual edit. The creation journal follows this event. After dispatch reacquire all data and re-run the strict complete proof; no stale pointer is reused across callbacks. There is one repair attempt, never a loop.

Recognized continuous creation identity events bypass generic transfer-chain/name invalidation until final verification. They do not erase unrelated reserved performance/name evidence at the temporarily reused engine ID. Unrelated or invalid transitions continue to invalidate evidence and stop generation.

Tests cover the observed4->3->4 sequence, health1.1/1.2/1.3712345, refusal of preexisting/duplicate/wrong-health candidates, occupied or reserved repair destinations, and unchanged strict final rejection of a reserved ID. No new hooks, polling, storage format or ordinary ApplyEquipment changes.

The runtime input is preserved separately in outputs/smithing-error-investigation-20261117/OutfitGallery-before-test3.log. Runtime alternation, side effects of notification callbacks and cleanup still require user confirmation. Existing untracked copies from failed older trials are not retrospectively adopted/deleted.
