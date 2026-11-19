# Enchantment reference diagnostics trial

Base release 2.2.6; metadata 2.2.6.8. Actual preset inspection (read-only) found Fur Boots recipe with 27 effects, 6 sourced from disabled SCBarrels.esp, whereas base ring is already omitted by missing-base filtering. This is a missing enchantment provider, not an absent outfit armor or release-specific regression.

Portable Resolve keeps its validation/lookup behavior but includes the missing plugin name, or plugin/localID for a changed reference. Localize the new dynamic prefixes and legacy generic inactive-plugin message. Status tests preserve names across EN/JP/external/fallback. No silent effect filtering, factory change, preset mutation or load-order modification. Missing-base filtering and original safeguards remain. 61 selected tests; runtime display pending.
