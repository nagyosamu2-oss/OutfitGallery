#include "EquipmentPluginCatalog.h"
#include <cassert>

int main() {
    using namespace Gallery::RandomTryOn;
    EquipmentPluginCatalog catalog;
    // One conversion patch overrides two original mods and introduces a new
    // accessory. It must be selectable once, with all three records available.
    AddEquipmentPlugin(catalog, "Dress.esp", "UBE Patch.esp", true, 0x100001);
    AddEquipmentPlugin(catalog, "Boots.esp", "UBE Patch.esp", false, 0x200001);
    AddEquipmentPlugin(catalog, "UBE Patch.esp", "UBE Patch.esp", false, 0x300001);
    AddEquipmentPlugin(catalog, "Jewelry.esp", "Jewelry.esp", false, 0x400001);
    assert(catalog.size() == 4);
    assert(catalog.at("UBE Patch.esp").count == 3);
    assert(catalog.at("UBE Patch.esp").tryOnBody);
    assert(catalog.at("Dress.esp").count == 1);
    assert(!catalog.at("Jewelry.esp").tryOnBody);
    assert(catalog.at("UBE Patch.esp").aliases.contains("Dress.esp"));
    assert(MatchEquipmentPlugin("Dress.esp", "UBE Patch.esp", "ube patch.ESP", true) == "UBE Patch.esp");
    assert(MatchEquipmentPlugin("Boots.esp", "UBE Patch.esp", "UBE Patch.esp", true) == "UBE Patch.esp");
    assert(MatchEquipmentPlugin("UBE Patch.esp", "UBE Patch.esp", "UBE Patch.esp", true) == "UBE Patch.esp");
    assert(MatchEquipmentPlugin("Dress.esp", "Later Fix.esp", "UBE Patch.esp", true).empty());
    assert(MatchEquipmentPlugin("Dress.esp", "UBE Patch.esp", "dress.ESP", true) == "Dress.esp");
    assert(MatchEquipmentPlugin("Dress.esp", "UBE Patch.esp", "patch", false) == "UBE Patch.esp");
    assert(MatchEquipmentPlugin("Dress.esp", "UBE Patch.esp", "patch", true).empty());
    assert(MatchEquipmentPlugin("Jewelry.esp", "Jewelry.esp", "jewel", false) == "Jewelry.esp");
    assert(MatchEquipmentPlugin("Dress.esp", "UBE Patch.esp", "", false) == "Dress.esp");

    // Different content (including patch-only additions) must not disappear.
    auto distinct = CollapseEquipmentPlugins(catalog);
    assert(distinct.size() == 4);
    assert(distinct.at("UBE Patch.esp").count == 3);

    EquipmentPluginCatalog identical;
    AddEquipmentPlugin(identical, "Base.esp", "UBE.esp", true, 0x100001);
    AddEquipmentPlugin(identical, "Base.esp", "UBE.esp", false, 0x100002);
    // Repeated discovery never inflates the set or winner preference.
    AddEquipmentPlugin(identical, "Base.esp", "UBE.esp", false, 0x100002);
    assert(identical.at("UBE.esp").winningCount == 2);
    auto collapsed = CollapseEquipmentPlugins(identical);
    assert(collapsed.size() == 1 && collapsed.contains("UBE.esp"));
    assert(collapsed.at("UBE.esp").count == 2);
    assert(collapsed.at("UBE.esp").aliases.contains("Base.esp"));
    assert(collapsed.at("UBE.esp").tryOnBody);
    // Input is retained, and running on an already-collapsed snapshot is stable.
    assert(identical.size() == 2);
    assert(CollapseEquipmentPlugins(collapsed).size() == 1);

    // Equal item counts and equal local IDs in different origins are not equal
    // runtime FormIDs. Both source plugins must remain visible.
    EquipmentPluginCatalog equalCounts;
    AddEquipmentPlugin(equalCounts, "A.esp", "A.esp", false, 0x100001);
    AddEquipmentPlugin(equalCounts, "B.esp", "B.esp", false, 0x200001);
    assert(CollapseEquipmentPlugins(equalCounts).size() == 2);

    // A shared final patch covering two origins remains because its combined
    // set differs from each origin, even when one origin contains only jewelry.
    EquipmentPluginCatalog combined;
    AddEquipmentPlugin(combined, "A.esp", "Final.esp", true, 0x100001);
    AddEquipmentPlugin(combined, "B.esp", "Final.esp", false, 0x200001);
    assert(CollapseEquipmentPlugins(combined).size() == 3);
    assert(CollapseEquipmentPlugins({}).empty());
}
