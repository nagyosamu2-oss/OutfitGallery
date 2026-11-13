#pragma once
#include "RandomTryOnNames.h"
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace Gallery::RandomTryOn {
// An override retains its original FormID. Grouping under a patch changes only
// the user's selection; resolving/acquiring still uses the defining plugin.
inline std::string MatchEquipmentPlugin(std::string_view origin, std::string_view winner,
                                       std::string_view query, bool exact) {
    const auto lowerQuery = LowerAscii(query);
    const auto matches = [&](std::string_view name) {
        if (name.empty()) return false;
        const auto lowerName = LowerAscii(name);
        return exact ? lowerName == lowerQuery : lowerName.find(lowerQuery) != std::string::npos;
    };
    if (matches(origin)) return std::string(origin);
    if (matches(winner)) return std::string(winner);
    return {};
}

struct EquipmentPluginEntry {
    std::size_t count{};
    bool tryOnBody{};
    std::set<std::string> aliases;
    std::set<std::uint32_t> items;
    std::size_t winningCount{};
};
using EquipmentPluginCatalog = std::map<std::string, EquipmentPluginEntry>;

inline void AddEquipmentPlugin(EquipmentPluginCatalog& catalog, std::string_view origin,
                               std::string_view winner, bool tryOnBody, std::uint32_t formID) {
    const auto add = [&](std::string_view name, std::string_view alias) {
        if (name.empty()) return;
        auto& entry = catalog[std::string(name)];
        const bool inserted = entry.items.insert(formID).second;
        entry.count = entry.items.size();
        if (inserted && name == winner) ++entry.winningCount;
        entry.tryOnBody |= tryOnBody;
        if (!alias.empty()) entry.aliases.emplace(alias);
    };
    add(origin, winner);
    if (winner != origin) add(winner, origin);
}

// Compare exact acquisition sets once, while building the dropdown snapshot.
// Count-only matches are insufficient: equally sized but different sets remain.
inline EquipmentPluginCatalog CollapseEquipmentPlugins(EquipmentPluginCatalog catalog) {
    std::map<std::set<std::uint32_t>, std::vector<std::string>> groups;
    for (const auto& [name, entry] : catalog)
        if (!entry.items.empty()) groups[entry.items].push_back(name);
    for (const auto& [items, names] : groups) {
        if (names.size() < 2) continue;
        const auto best = std::max_element(names.begin(), names.end(), [&](const auto& a, const auto& b) {
            const auto left = catalog.at(a).winningCount, right = catalog.at(b).winningCount;
            return left != right ? left < right : a > b;
        });
        auto& representative = catalog.at(*best);
        for (const auto& name : names) {
            if (name == *best) continue;
            const auto& hidden = catalog.at(name);
            representative.aliases.insert(name);
            representative.aliases.insert(hidden.aliases.begin(), hidden.aliases.end());
            representative.tryOnBody |= hidden.tryOnBody;
            catalog.erase(name);
        }
    }
    return catalog;
}
}
