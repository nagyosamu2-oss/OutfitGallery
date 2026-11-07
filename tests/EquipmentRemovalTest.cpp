#include "EquipmentRemoval.h"
#include <iostream>
using namespace Gallery;
void require(bool b){if(!b)throw std::runtime_error("Equipment removal selection regression");}
template<class F> void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}require(bad);}
int main(){try{
    std::vector<RemovalItem> rows{{100,4,"Clothes"},{200,1<<14,"First accessory"},{300,1<<15,"Second accessory"}};
    require(SelectedRemovalIDs(rows,{1})==std::vector<std::uint32_t>{200});
    require(SelectedRemovalIDs(rows,{0,2})==std::vector<std::uint32_t>({100,300}));
    rejects([&]{SelectedRemovalIDs(rows,{});});
    rejects([&]{SelectedRemovalIDs(rows,{3});});
    require(RemovalSelectionStillCurrent(rows,{1},rows));
    auto current=rows;current.erase(current.begin()+1);
    require(!RemovalSelectionStillCurrent(rows,{1},current)); // selected item changed
    require(RemovalSelectionStillCurrent(rows,{2},current)); // unrelated item changed
    current=rows;current[1].slots=1<<16;
    require(!RemovalSelectionStillCurrent(rows,{1},current));
    require(!RemovalSelectionStillCurrent(rows,{4},rows));
    // Keep stable rows while two items are removed/restored independently.
    const auto baseline=rows;
    require(RemovalCurrentMatches(rows,{},baseline));
    current={baseline[0],baseline[2]};
    require(RemovalCurrentMatches(rows,{1},current));
    require(!RemovalCurrentMatches(rows,{},current));
    current={baseline[0]};
    require(RemovalCurrentMatches(rows,{1,2},current));
    current={baseline[0],baseline[1]};
    require(RemovalCurrentMatches(rows,{2},current)); // first item restored
    require(!RemovalCurrentMatches(rows,{1},current));
    require(RemovalCurrentMatches(rows,{},baseline)); // both restored
    require(RemovalCurrentMatches(rows,{0,1,2},{}));
    require(!RemovalCurrentMatches(rows,{4},baseline));
    current=baseline;current[1].slots=1<<20;
    require(!RemovalCurrentMatches(rows,{},current));
    // Large registration libraries are irrelevant; only currently worn rows are used.
    rows.push_back({0xFF001234,0,"Dynamic armor"});
    require(SelectedRemovalIDs(rows,{3})==std::vector<std::uint32_t>{0xFF001234});
    std::cout<<"PASS: exact subset, empty/invalid selection, stale item isolation and dynamic IDs\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
