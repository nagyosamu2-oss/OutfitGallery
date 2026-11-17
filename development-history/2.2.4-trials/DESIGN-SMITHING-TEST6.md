# Saved quality labels and organizer target ordering

Base test5, metadata2.2.4.6. Two display changes only.

ResolvePresetDisplayNames is called only in the existing game-thread scope request10 handler after slot resolution. For saved armor smithing, instantiate a temporary RE::ExtraTextDisplayData and copy its GetDisplayName(baseArmor,savedHealth) result into owned strings before destruction. No inventory extra-list, actor equipment, base armor, snapshot or preset file is modified. UI reads the strings under presetsMutex with the existing ready flag. Clear labels on new request; fall back to saved base name if no label is available. Full, partial and slotless management rows consume the same per-index labels. Scope edit/selection identity keys are unchanged.

RegistrationTabs returns copies in stable normal-then-accessory order. Organizer category switch uses existing CollectionTabs and its draft. Width calculation, visible target indexing, pad targetFocus and context move destinations all use the ordered target vector. Storage membership/order is never changed. Built-in head is a view/scope, not an existing user registration category, so the registration row still includes only eligible categories. CollectionTabs already places normal/head/accessory in the main view order.

Automated category cases include interleaved creation order (normal/accessory/accessory/later-normal), expected target ordering, unchanged underlying order, empty library, and existing main view built-in head placement. Existing equipment/generation/reclaim sources are byte compared; Equipment.cpp has one added display resolver with the rest unchanged. Runtime engine quality suffixes and widget layout still require user confirmation.
