#pragma once
#include "Presets.h"
#include <set>
namespace Gallery::Variations {
// Membership is metadata only. The first photo is the preferred representative.
inline void Remove(Library& l,const std::set<std::string>& photos) {
    for(auto& group:l.variations) std::erase_if(group,[&](const auto& p){return photos.contains(p);});
    std::erase_if(l.variations,[](const auto& g){return g.size()<2;});
}
inline void Ungroup(Library& l,const std::set<std::string>& photos) {
    Remove(l,photos); // Keep unselected members grouped; dissolve only singletons.
}
inline bool CanGroup(const std::vector<Preset>& cards,const std::set<std::string>& selected) {
    const Preset* first=nullptr;unsigned count=0;
    for(const auto& p:cards) if(selected.contains(p.photo)) {
        if(first && (first->accessories!=p.accessories || first->slotMask!=p.slotMask)) return false;
        first=&p;++count;
    }
    return count>=2 && count<=100;
}
inline bool Group(Library& l,const std::vector<Preset>& cards,const std::set<std::string>& selected) {
    if(!CanGroup(cards,selected)) return false;
    std::vector<std::string> group;
    for(const auto& p:cards) if(selected.contains(p.photo)) group.push_back(p.photo);
    Remove(l,{group.begin(),group.end()});l.variations.push_back(std::move(group));return true;
}
inline bool GroupAndClearSelection(Library& l,const std::vector<Preset>& cards,
    const std::set<std::string>& targets,std::set<std::string>& selection) {
    if(!Group(l,cards,targets)) return false;
    selection.clear();return true;
}
inline void UngroupAndClearSelection(Library& l,const std::set<std::string>& targets,std::set<std::string>& selection) {
    Ungroup(l,targets);selection.clear();
}
inline std::map<std::string,std::vector<Preset>> Collapse(std::vector<Preset>& cards,const Library& l) {
    std::map<std::string,std::vector<Preset>> groups;
    std::set<std::string> hidden;
    std::map<std::string,const Preset*> lookup;
    for(const auto& p:cards) lookup.emplace(p.photo,&p);
    for(const auto& group:l.variations) {
        std::vector<Preset> visible;
        for(const auto& photo:group) if(auto it=lookup.find(photo);it!=lookup.end()) visible.push_back(*it->second);
        if(visible.size()<2) continue;
        for(std::size_t i=1;i<visible.size();++i) hidden.insert(visible[i].photo);
        const auto representative=visible.front().photo;groups.emplace(representative,std::move(visible));
    }
    std::erase_if(cards,[&](const auto& p){return hidden.contains(p.photo);});return groups;
}
struct HoverClock {
    std::string candidate;std::uint64_t since{},outside{};
    bool Ready(const std::string& photo,std::uint64_t now) {
        if(candidate!=photo){candidate=photo;since=now;return false;}
        return now-since>=250;
    }
    bool Leave(bool inside,std::uint64_t now) {
        if(inside){outside=0;return false;}
        if(!outside)outside=now;
        return now-outside>=100;
    }
};
struct Interaction {
    std::string photo;HoverClock clock;bool pad{},context{};int index{};
    template<class Groups> bool Reconcile(const Groups& groups) {
        // An empty active photo means the dwell is still accumulating.
        // Clearing that state every frame prevents any hover from opening.
        return photo.empty() || groups.contains(photo);
    }
};
// Dragging a collapsed representative always carries its complete group.
// Preserve other selected cards only when the grabbed representative was selected.
inline std::set<std::string> DragSelection(const std::set<std::string>& selection,
    const std::string& photo,const std::map<std::string,std::vector<Preset>>& groups) {
    auto result=selection.contains(photo)?selection:std::set<std::string>{};
    if(const auto it=groups.find(photo);it!=groups.end())
        for(const auto& p:it->second)result.insert(p.photo);
    else result.insert(photo);
    return result;
}
inline void ToggleSelection(std::set<std::string>& selection,const std::string& photo,
    const std::map<std::string,std::vector<Preset>>& groups) {
    const auto it=groups.find(photo);
    if(it==groups.end()) {if(!selection.erase(photo))selection.insert(photo);return;}
    const bool all=std::all_of(it->second.begin(),it->second.end(),[&](const auto& p){return selection.contains(p.photo);});
    for(const auto& p:it->second) {if(all)selection.erase(p.photo);else selection.insert(p.photo);}
}
}
