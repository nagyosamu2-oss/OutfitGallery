#pragma once
#include <algorithm>
#include <set>
#include <string>
#include <vector>
namespace Gallery::Organize {
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
