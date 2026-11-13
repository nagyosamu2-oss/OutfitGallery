#include "EquipmentRemoval.h"
#include <atomic>
#include <mutex>
#include <iostream>
namespace RE {struct Actor {unsigned GetFormID() const{return 0x14;}};}
using namespace Gallery;
std::atomic<unsigned> studioOpenSerial{1};
std::atomic<bool> removalDummy{},saveBusy{};
unsigned verificationTicks{};unsigned long long equipmentLaneDue{};
bool appearance{},autoAppearanceOwned{},noEffect{};
unsigned mutations{};unsigned long long now{};
std::vector<RemovalItem> current,baseline;
auto GetTickCount64(){return now;}
bool TargetAppearanceMode(unsigned){return appearance;}
bool IsFollowerOutfitMaintained(RE::Actor*){return false;}
void RememberFollowerOutfit(RE::Actor*){}
void RefreshFollowerAfterAccessoryRemoval(RE::Actor*){}
void SetTransmogAutoOwned(RE::Actor*,bool){}
void AutoManualChange(RE::Actor*){}
void SetStatus(const char*){}
namespace Gallery {
std::vector<RemovalItem> SnapshotRemovalEquipment(RE::Actor*){return current;}
std::vector<RemovalItem> SnapshotRemovalAppearance(RE::Actor*){return current;}
void BeginRemovalEquipment(RE::Actor*){}
void BeginRemovalAppearance(RE::Actor*){}
bool RemoveEquipmentIDs(RE::Actor*,const std::vector<std::uint32_t>& ids) {
 ++mutations;if(!noEffect)std::erase_if(current,[&](const auto& r){return std::find(ids.begin(),ids.end(),r.id)!=ids.end();});return true;
}
bool RemoveAppearanceIDs(RE::Actor* a,const std::vector<std::uint32_t>& ids){return RemoveEquipmentIDs(a,ids);}
bool RestoreRemovalEquipment(RE::Actor*,std::uint32_t id) {
 ++mutations;auto it=std::find_if(baseline.begin(),baseline.end(),[&](const auto& r){return r.id==id;});
 if(it==baseline.end())return false;if(!noEffect)current.push_back(*it);return true;
}
bool RestoreRemovalAppearance(RE::Actor* a,std::uint32_t id){return RestoreRemovalEquipment(a,id);}
}
#include "EquipmentRemovalRuntime.inc"
void check(bool b){if(!b)throw std::runtime_error("batch removal queue regression");}
RE::Actor actor;
void reset(bool visual) {
 appearance=visual;current=baseline={{100,4,"Body"},{200,8,"Hands"},{300,16,"Feet"}};
 removalState={};removalState.session=1;removalState.actor=0x14;removalState.appearance=visual;
 studioOpenSerial=1;mutations=0;now=0;noEffect=false;ProcessEquipmentRemoval(34,&actor);
}
void start(bool remove) {
 auto rows=PendingRemovalRows(removalState.rows,removalState.removed,remove);check(!rows.empty());
 removalState.actionRow=rows.front();removalState.queuedRows.assign(rows.begin()+1,rows.end());removalState.removeWanted=remove;
 ProcessEquipmentRemoval(35,&actor);
}
void finish() {for(int i=0;i<10 && removalPending;++i){now+=16;ProcessEquipmentRemoval(0,&actor);}check(!removalPending);}
int main(){try{
 for(bool visual:{false,true}) {
  reset(visual);start(true);finish();check(current.empty() && mutations==3 && removalState.removed.size()==3);
  start(false);finish();check(RemovalCurrentMatches(baseline,{},current) && mutations==6);
  reset(visual);removalState.actionRow=1;removalState.removeWanted=true;ProcessEquipmentRemoval(35,&actor);finish();
  start(true);finish();check(current.empty() && mutations==3);start(false);finish();check(current.size()==3 && mutations==6);
  reset(visual);start(true);current.back().slots=999;finish();check(mutations==1 && !removalState.error.empty() && removalState.queuedRows.empty());
  reset(visual);noEffect=true;start(true);now=1000;finish();check(mutations==1 && !removalState.error.empty() && removalState.queuedRows.empty());
  reset(visual);start(true);++studioOpenSerial;finish();check(mutations==1 && !removalState.error.empty());
 }
 std::cout<<"PASS batch remove/restore, partial selection, stale state, timeout and session cancellation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
