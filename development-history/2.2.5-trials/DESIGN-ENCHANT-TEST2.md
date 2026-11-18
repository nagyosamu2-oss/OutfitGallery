# Enchantment test2 — runtime cost-override normalization

Captured test1 restart log: effects and all remaining fields match; expected costOverride 438/442, flags1 vs factory costOverride -1, flags0. Raw log preserved in outputs/enchantment-probe-evidence-20261118/enchant-test1-price-override-mismatch.log. The saved recipe was read; failure occurred before inventory addition at full metadata equality.

Normalize only ENIT costOverride (index2) and EnchantmentFlag::kCostOverride (bit0 of index3) when comparing performance. The active override must still have a nonnegative int32 cost; unknown/unsupported values fail conservatively. All other flags and metadata remain exact. FactoryEquivalent also requires equal smithing health; SameEnchant ignores health only because cleanup separately protects additional smithing. Both parse bounded recipes and reject invalid data.

Use FactoryEquivalent at factory validation, fresh-list check, and identity proof. Use the same normalized comparison during baseline cleanup, so a newly reproduced item whose price changed is not treated as a later player modification. Original recipe strings and EPRF layout remain unchanged. Never mutate a potentially shared dynamic ENCH to force its gold value to match. Display/sale price may differ from the source. Scope remains constant/self armor, no weapon charges or spell-casting semantics are generalized.

Regression tests replay the exact 438/flags1 vs -1/flags0 mismatch in both directions; permit other valid price overrides, reject effect magnitude changes, different smithing during factory proof, different casting/delivery/spell type, charges, base/restriction references, non-price flags, temporary state and malformed signatures. Health-only change remains comparable as enchantment but protected separately as before.

Native code outside PortableEnchantment comparison, fresh-list/evidence comparison and version labels is unchanged from test1. Preset serialization, cleanup ownership/UID claims, EPRF persistence and original-item selection are unchanged. Earlier user reports confirm manual modification preservation/reuse; integrated restart/new-game testing remains pending.
