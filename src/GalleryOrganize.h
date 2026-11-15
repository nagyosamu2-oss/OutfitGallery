#pragma once
#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include "Presets.h"
namespace Gallery::Organize {
// Match the destination tab type without changing the selected preset's scope.
// Return identities so registering never changes the selection or photo order.
inline std::vector<std::string> CategoryTargets(const Library& draft,
        const std::vector<Preset>& visible,const std::set<std::string>& selected,bool accessories=false) {
    std::vector<std::string> targets;
    std::set<std::string> seen;
    for(const auto& preset:visible) {
        if(preset.accessories!=accessories || (!accessories && preset.slotMask) || !selected.contains(preset.photo)) continue;
        const auto label=draft.labels.find(preset.photo);
        if(label!=draft.labels.end() && label->second.deleted) continue;
        if(seen.insert(preset.photo).second) targets.push_back(preset.photo);
    }
    return targets;
}
inline std::size_t AddToCategory(Library& draft,const std::vector<std::string>& targets,const std::string& category) {
    if(category.empty() || category.starts_with("#") ||
       std::none_of(draft.categories.begin(),draft.categories.end(),[&](const auto& c){return c.id==category;})) return 0;
    std::size_t added=0;
    for(const auto& photo:targets) {
        auto& label=draft.labels[photo];
        if(label.deleted) continue;
        if(std::find(label.categories.begin(),label.categories.end(),category)==label.categories.end()) {
            label.categories.push_back(category);++added;
        }
    }
    return added;
}
inline bool IsCategory(const Library& draft,const std::string& category) {
    return !category.empty() && !category.starts_with("#") &&
        std::any_of(draft.categories.begin(),draft.categories.end(),[&](const auto& c){return c.id==category;});
}
// Remove only membership in the current editable tab. Keep the label itself,
// other memberships, order, equipment choices and Trash state unchanged.
inline std::size_t RemoveFromCategory(Library& draft,const std::vector<std::string>& targets,const std::string& category) {
    if(!IsCategory(draft,category)) return 0;
    std::size_t removed=0;
    for(const auto& photo:targets) {
        auto label=draft.labels.find(photo);
        if(label==draft.labels.end() || label->second.deleted) continue;
        if(std::erase(label->second.categories,category))++removed;
    }
    return removed;
}
// A selected context item acts on the visible selection; an unselected one
// Transfer membership, not the photo or other tab memberships. Invalid and
// cross-type destinations are rejected before changing any labels.
inline bool CanMoveToCategory(const Library& draft,const std::string& source,const std::string& destination) {
    if(source==destination || !IsCategory(draft,source) || !IsCategory(draft,destination)) return false;
    const auto from=std::find_if(draft.categories.begin(),draft.categories.end(),[&](const auto& c){return c.id==source;});
    const auto to=std::find_if(draft.categories.begin(),draft.categories.end(),[&](const auto& c){return c.id==destination;});
    return from->accessories==to->accessories;
}
inline std::size_t MoveToCategory(Library& draft,const std::vector<std::string>& targets,
        const std::string& source,const std::string& destination) {
    if(!CanMoveToCategory(draft,source,destination)) return 0;
    std::size_t moved=0;
    for(const auto& photo:targets) {
        auto it=draft.labels.find(photo);
        if(it==draft.labels.end() || it->second.deleted) continue;
        auto& tabs=it->second.categories;
        if(std::find(tabs.begin(),tabs.end(),source)==tabs.end()) continue;
        // Add first, so allocation failure cannot unregister an outfit.
        if(std::find(tabs.begin(),tabs.end(),destination)==tabs.end()) tabs.push_back(destination);
        std::erase(tabs,source);++moved;
    }
    return moved;
}

// A selected context item acts on the visible selection; an unselected one
// acts only on itself. Empty context denotes the toolbar selection action.
inline std::vector<std::string> TrashTargets(const std::vector<std::string>& visible,
        const std::set<std::string>& selected,const std::string& context={}) {
    std::vector<std::string> result;
    const bool group=context.empty() || selected.contains(context);
    for(const auto& id:visible) if(group?selected.contains(id):id==context) result.push_back(id);
    return result;
}
// Replace only the slots occupied by the visible subset. Hidden photos never move.
inline bool Move(std::vector<std::string>& order,const std::vector<std::string>& visible,
                 const std::set<std::string>& selected,const std::string& target={},bool after=false,bool fixedSlot=false) {
    std::set<std::string> view(visible.begin(),visible.end());
    if(view.size()!=visible.size()) return false;
    std::vector<std::size_t> slots;
    std::vector<std::string> moving,remaining;
    std::set<std::string> found;
    for(std::size_t i=0;i<order.size();++i) if(view.contains(order[i])) {
        if(!found.insert(order[i]).second) return false;
        slots.push_back(i);
        (selected.contains(order[i])?moving:remaining).push_back(order[i]);
    }
    if(found!=view || moving.empty()) return false;
    std::size_t at=after?remaining.size():0;
    if(!target.empty()) {
        if(selected.contains(target)) return false;
        const auto it=std::find(remaining.begin(),remaining.end(),target);
        if(it==remaining.end()) return false;
        at=static_cast<std::size_t>(it-remaining.begin())+(after?1:0);
        if(fixedSlot) {
            // Point at a visible grid position, not a photo that shifts on removal.
            std::size_t index=0;
            for(const auto slot:slots) {if(order[slot]==target)break;++index;}
            at=std::min(index,remaining.size());
        }
    }
    remaining.insert(remaining.begin()+at,moving.begin(),moving.end());
    bool changed=false;
    for(std::size_t i=0;i<slots.size();++i) {changed|=order[slots[i]]!=remaining[i];order[slots[i]]=remaining[i];}
    return changed;
}
inline void SelectRange(std::set<std::string>& selected,const std::vector<std::string>& view,
                        const std::string& anchor,const std::string& clicked) {
    auto a=std::find(view.begin(),view.end(),anchor),b=std::find(view.begin(),view.end(),clicked);
    if(b==view.end()) return;
    if(a==view.end()) {selected.insert(clicked);return;}
    if(a>b) std::swap(a,b);
    selected.insert(a,b+1);
}
}
