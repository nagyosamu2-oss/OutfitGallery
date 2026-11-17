# Generation verification revision

Base: 2.2.4-smithing-test1, confirmed UI checkpoint remains unchanged.

Reported evidence: user log contains success for form000A6D7F at health1.1, and five generic post-generation verification failures. Detailed identity events were absent. Cause is not established by that log; no unsupported attribution to another mod.

Post-add verification now directly identifies the submitted list's scalar transaction identity instead of calling the selection policy that prioritizes an original or retained copy. During the synchronous AddObject call, record at most four continuous same-owner unique-ID transitions. Reject unrelated or outgoing identity changes. Accept the resulting singleton only with before-count+1, new/unreserved identity, exact requested health, no custom enchantment and one matching list. Reserved IDs include pre-add raw entries, original-instance reservations and managed identities. Ordinary owned items never receive ExtraHealth.

No-op ID notifications do not discard baseline evidence. Actual ID changes invalidate baseline evidence regardless of logging state; this previously depended on detailed logging or cleanup compatibility. No ownership claim is inferred solely from a count delta or matching health.

On unverified creation relinquish the provisional claims and block further generation of that form while its count exceeds the pre-failure count for this session. Find also rejects the blocked form so a failed claim cannot be reused on the next click. This block is session-local, clears on load/revert and is not a persistent repair of existing untracked copies. Use the pre-test save for comparison.

Focused info logs during reproduction include begin, identity event, up to32 candidate lists and final scalar proof. They do not depend on the global detail toggle and do not run for ordinary presets. Runtime cause and efficacy remain unverified.

Added policy cases cover ordinary/fine/superior numeric health, direct singleton proof, pre-existing/reserved IDs, owner mismatch, count mismatch, stacks/duplicates, unknown transitions, no-op transitions and hop limit. All earlier native generation safeguards and the known-self-reclaim event guard remain.
