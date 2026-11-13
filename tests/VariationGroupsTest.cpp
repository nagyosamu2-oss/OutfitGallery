#include "VariationGroups.h"
#include <fstream>
#include <iostream>
using namespace Gallery;
void Check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
Preset Card(const char* photo,bool accessories=false,unsigned slots=0){Preset p;p.photo=photo;p.name=photo;p.accessories=accessories;p.slotMask=slots;return p;}
int main(){try {
    Library l;l.order={"a.png","b.png","c.png"};l.labels["a.png"].categories={"favorites"};l.categories={{"favorites","Favorites"}};
    std::vector<Preset> cards{Card("a.png"),Card("b.png"),Card("c.png"),Card("x.png",true,4),Card("h.png",false,2)};
    Check(!Variations::Group(l,cards,{"a.png"}),"single photo rejected");
    Check(!Variations::Group(l,cards,{"a.png","x.png"}),"mixed types rejected");
    Check(!Variations::Group(l,cards,{"a.png","h.png"}),"head scope rejected");
    auto draft=l;Check(Variations::Group(draft,cards,{"b.png","a.png"}),"group");
    Check(l.variations.empty(),"draft cancellation leaves live library");
    Check(draft.variations[0]==std::vector<std::string>({"a.png","b.png"}),"display order chooses representative");
    auto collapsed=cards;auto groups=Variations::Collapse(collapsed,draft);
    Check(collapsed.size()==4 && groups.at("a.png").size()==2,"collapse");
    std::vector<Preset> filtered{cards[1],cards[2]};
    Check(Variations::Collapse(filtered,draft).empty() && filtered.size()==2,"single surviving search match stays visible");
    Check(Variations::Group(draft,cards,{"b.png","c.png"}),"regroup");
    Check(draft.variations.size()==1 && draft.variations[0][0]=="b.png","membership unique after regroup");
    auto onlyGroup=std::vector<Preset>{cards[0],cards[2],cards[1]};groups=Variations::Collapse(onlyGroup,draft);
    Check(groups.at("b.png").size()==2 && onlyGroup.size()==2,"noncontiguous members");
    Check(draft.order==l.order && draft.labels.at("a.png").categories==l.labels.at("a.png").categories,"grouping preserves order and categories");
    auto path=std::filesystem::temp_directory_path()/"OutfitGallery-variation-groups-test"/"Library.json";
    WriteLibrary(draft,path);auto loaded=ReadLibrary(path);Check(loaded.variations==draft.variations,"persistence");
    auto bad=draft;bad.variations.push_back({"a.png","b.png"});bool rejected=false;
    try{WriteLibrary(bad,path);}catch(...){rejected=true;}
    Check(rejected && ReadLibrary(path).variations==draft.variations,"duplicate rejected without overwriting");
    bad=draft;bad.variations={{"../bad.png","a.png"}};rejected=false;try{WriteLibrary(bad,path);}catch(...){rejected=true;}Check(rejected,"unsafe identity rejected");
    Variations::Ungroup(loaded,{"b.png"});Check(loaded.variations.empty() && loaded.order==draft.order,"ungroup metadata only");
    WriteLibrary(l,path);Check(ReadLibrary(path).variations.empty(),"legacy optional field");
    Variations::Remove(draft,{"b.png"});Check(draft.variations.empty(),"delete cleans singleton");
    // Consecutive grouping must not carry the previous checked photos along.
    std::vector<Preset> batch{Card("a.png"),Card("b.png"),Card("c.png"),Card("d.png"),Card("e.png"),Card("f.png")};
    Library batches;std::set<std::string> checked{"a.png","b.png"};
    Check(Variations::GroupAndClearSelection(batches,batch,checked,checked) && checked.empty(),"successful toolbar grouping clears selection even with aliased targets");
    checked={"c.png","d.png"};const auto contextTargets=checked;
    Check(Variations::GroupAndClearSelection(batches,batch,contextTargets,checked) && checked.empty(),"context grouping clears selection");
    Check(batches.variations==std::vector<std::vector<std::string>>{{"a.png","b.png"},{"c.png","d.png"}},"consecutive groups remain independent");
    checked={"e.png"};Check(!Variations::GroupAndClearSelection(batches,batch,checked,checked) && checked.size()==1,"rejected grouping keeps selection");
    Library partial;partial.variations={{"a.png","b.png","c.png","d.png"},{"e.png","f.png"}};
    Variations::Ungroup(partial,{"b.png"});
    Check(partial.variations==std::vector<std::vector<std::string>>{{"a.png","c.png","d.png"},{"e.png","f.png"}},"detach only selected child and preserve other groups");
    Variations::Ungroup(partial,{"a.png"});
    Check(partial.variations[0]==std::vector<std::string>{"c.png","d.png"},"detaching representative promotes first remaining child");
    Variations::Ungroup(partial,{"c.png"});Check(partial.variations==std::vector<std::vector<std::string>>{{"e.png","f.png"}},"one remaining child becomes standalone");
    Variations::Ungroup(partial,{"e.png","f.png"});Check(partial.variations.empty(),"selecting all dissolves group");
    Library detachSelection;detachSelection.variations={{"a.png","b.png","c.png","d.png"}};
    checked={"b.png"};Variations::UngroupAndClearSelection(detachSelection,checked,checked);
    Check(checked.empty() && detachSelection.variations[0]==std::vector<std::string>{"a.png","c.png","d.png"},"toolbar detach clears selection with aliased targets");
    checked={"a.png"};const auto detachTargets=checked;
    Variations::UngroupAndClearSelection(detachSelection,detachTargets,checked);
    Check(checked.empty() && detachSelection.variations[0]==std::vector<std::string>{"c.png","d.png"},"context detach clears selection and keeps remaining group");
    Variations::HoverClock clock;
    Variations::Interaction interaction;
    // Regression: normal frames with no open strip must preserve accumulated dwell.
    const std::map<std::string,std::vector<Preset>> available{{"a",{cards[0],cards[1]}}};
    for(unsigned t=1000;t<1250;t+=10) {
        if(!interaction.Reconcile(available))interaction={};
        Check(!interaction.clock.Ready("a",t),"dwell not ready before deadline");
    }
    Check(interaction.Reconcile(available) && interaction.clock.Ready("a",1250),"empty active photo must not reset dwell between frames");
    interaction.photo="missing";Check(!interaction.Reconcile(available),"missing active group invalidates interaction");
    auto selection=std::set<std::string>{};
    auto groupedCards=cards;auto selectable=Variations::Collapse(groupedCards,ReadLibrary(path));
    // path is a legacy library at this point; construct an active group explicitly.
    Library selectionLibrary;Variations::Group(selectionLibrary,cards,{"a.png","b.png"});
    groupedCards=cards;selectable=Variations::Collapse(groupedCards,selectionLibrary);
    Variations::ToggleSelection(selection,"a.png",selectable);
    Check(selection==std::set<std::string>({"a.png","b.png"}),"collapsed card selects all members");
    Variations::ToggleSelection(selection,"a.png",selectable);Check(selection.empty(),"collapsed card deselects all members");
    Variations::ToggleSelection(selection,"b.png",selectable);Check(selection==std::set<std::string>{"b.png"},"expanded child selects independently");
    Variations::ToggleSelection(selection,"a.png",selectable);Check(selection.size()==2,"partially selected group selects remainder");
    Check(Variations::DragSelection({},"a.png",selectable)==std::set<std::string>({"a.png","b.png"}),"drag unselected representative carries complete group");
    Check(Variations::DragSelection({"a.png","c.png"},"a.png",selectable)==std::set<std::string>({"a.png","b.png","c.png"}),"drag partial group fills members and preserves other selection");
    Check(Variations::DragSelection({"c.png"},"a.png",selectable)==std::set<std::string>({"a.png","b.png"}),"grab unselected group replaces unrelated selection");
    Check(!clock.Ready("a",1000) && !clock.Ready("a",1249) && clock.Ready("a",1250),"hover dwell");
    Check(!clock.Ready("b",1251),"moving across groups resets dwell");
    Check(!clock.Leave(false,2000) && !clock.Leave(false,2099),"close grace");
    Check(!clock.Leave(true,2100),"crossing into strip cancels close");
    Check(!clock.Leave(false,2200) && clock.Leave(false,2300),"close after 100 ms outside");
    std::cout<<"Variation grouping, filtering, persistence and hover timing passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
