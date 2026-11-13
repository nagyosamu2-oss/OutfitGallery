#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
namespace RE {class Actor;}
namespace Gallery {
struct RemovalItem {
    std::uint32_t id{},slots{};
    std::string name;
    bool operator==(const RemovalItem&) const = default;
};
inline std::vector<std::uint32_t> SelectedRemovalIDs(const std::vector<RemovalItem>& rows,const std::set<unsigned>& chosen) {
    if(chosen.empty()) throw std::runtime_error("Select equipment to remove.");
    std::vector<std::uint32_t> ids;
    for(auto n:chosen) {
        if(n>=rows.size() || !rows[n].id) throw std::runtime_error("Equipment changed. Refresh the list.");
        ids.push_back(rows[n].id);
    }
    return ids;
}
inline bool RemovalSelectionStillCurrent(const std::vector<RemovalItem>& rows,const std::set<unsigned>& chosen,const std::vector<RemovalItem>& current) {
    if(chosen.empty()) return false;
    for(auto n:chosen) if(n>=rows.size() || std::find(current.begin(),current.end(),rows[n])==current.end()) return false;
    return true;
}
inline bool RemovalCurrentMatches(const std::vector<RemovalItem>& rows,const std::set<unsigned>& removed,const std::vector<RemovalItem>& current) {
    if(std::any_of(removed.begin(),removed.end(),[&](auto n){return n>=rows.size();})) return false;
    if(current.size()!=rows.size()-removed.size()) return false;
    for(unsigned n=0;n<rows.size();++n)
        if(!removed.contains(n) && std::find(current.begin(),current.end(),rows[n])==current.end()) return false;
    return true;
}
// Queue only rows that need changing; stable indices preserve per-item restore data.
inline std::vector<unsigned> PendingRemovalRows(const std::vector<RemovalItem>& rows,const std::set<unsigned>& removed,bool remove) {
    std::vector<unsigned> result;
    for(unsigned n=0;n<rows.size();++n) if(removed.contains(n)!=remove) result.push_back(n);
    return result;
}
std::vector<RemovalItem> SnapshotRemovalEquipment(RE::Actor*);
std::vector<RemovalItem> SnapshotRemovalAppearance(RE::Actor*);
bool RemoveEquipmentIDs(RE::Actor*,const std::vector<std::uint32_t>&);
bool EquipmentIDsRemoved(RE::Actor*,const std::vector<std::uint32_t>&);
void BeginRemovalEquipment(RE::Actor*);
bool RestoreRemovalEquipment(RE::Actor*,std::uint32_t);
void BeginRemovalAppearance(RE::Actor*);
bool RestoreRemovalAppearance(RE::Actor*,std::uint32_t);
bool RemoveAppearanceIDs(RE::Actor*,const std::vector<std::uint32_t>&);
}
