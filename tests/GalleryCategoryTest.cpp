#include "GalleryOrganize.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void Check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
int main(){try {
    Library committed;
    committed.categories={{"favorites","Favorites"},{"daily","Daily"},{"tab-last","作成したタブ"}};
    committed.order={"a.png","hidden.png","b.png","trash.png","head.png","accessory.png"};
    committed.labels["a.png"].name="Renamed outfit";
    committed.labels["a.png"].categories={"favorites"};
    committed.labels["a.png"].exchangeSlots=4;
    committed.labels["a.png"].outfitItems=std::vector<std::string>{"Example.esp:123"};
    committed.labels["b.png"].categories={"daily"};
    committed.labels["trash.png"].deleted=true;
    std::vector<Preset> visible;
    for(const auto* name:{"a.png","b.png","trash.png","head.png","accessory.png"}) {
        Preset p;p.name=name;p.photo=name;visible.push_back(p);
    }
    visible[3].slotMask=1;visible[4].accessories=true;
    const std::set<std::string> selected(committed.order.begin(),committed.order.end());
    auto draft=committed;
    const auto targets=Organize::CategoryTargets(draft,visible,selected);
    Check(targets==std::vector<std::string>({"a.png","b.png"}),"only visible selected full outfits are registered");
    Check(Organize::AddToCategory(draft,targets,"daily")==1,"mixed new and existing membership");
    Check(Organize::AddToCategory(draft,targets,"daily")==0,"repeated drop is idempotent");
    Check(Organize::AddToCategory(draft,targets,"tab-last")==2,"same selection can register to another tab");
    Check(draft.labels.at("a.png").categories==std::vector<std::string>({"favorites","daily","tab-last"}),"old memberships are retained");
    Check(Organize::CategoryTargets(draft,visible,selected)==targets,"selection remains usable after registration");
    Check(draft.order==committed.order,"registration does not reorder photos");
    Check(!draft.labels.contains("hidden.png") && !draft.labels.contains("head.png") && !draft.labels.contains("accessory.png"),"unrelated entries are untouched");
    Check(draft.labels.at("trash.png").deleted && draft.labels.at("trash.png").categories.empty(),"Trash is not restored or classified");
    Check(draft.labels.at("a.png").name=="Renamed outfit" && draft.labels.at("a.png").exchangeSlots==4 && draft.labels.at("a.png").outfitItems==committed.labels.at("a.png").outfitItems,"registered equipment and names preserved");
    for(const auto* invalid:{"","#trash","#head","nonexistent"}) Check(Organize::AddToCategory(draft,targets,invalid)==0,"invalid targets do not create tabs");
    Check(committed.labels.at("a.png").categories==std::vector<std::string>({"favorites"}),"cancelled draft leaves committed state unchanged");

    // Exercise the actual library writer/reader used by Save and close.
    const auto folder=std::filesystem::current_path()/"gallery-category-test";
    std::filesystem::create_directories(folder);
    const auto path=folder/"Library.json";
    WriteLibrary(committed,path);
    auto reloaded=ReadLibrary(path);
    Check(reloaded.labels.at("a.png").categories==committed.labels.at("a.png").categories,"draft remains unsaved on disk");
    WriteLibrary(draft,path);reloaded=ReadLibrary(path);
    Check(reloaded.labels.at("a.png").categories==draft.labels.at("a.png").categories && reloaded.labels.at("b.png").categories==draft.labels.at("b.png").categories,"batch registration survives reopening");
    Check(reloaded.order==committed.order && reloaded.labels.at("a.png").outfitItems==committed.labels.at("a.png").outfitItems,"persistence preserves order and equipment selection");

    // Right-clicking a selected photo removes the visible selection; clicking
    // an unselected photo affects only that photo. Neither path deletes files.
    auto removalDraft=draft;
    const std::vector<std::string> currentTab{"a.png","b.png"};
    const auto batch=Organize::TrashTargets(currentTab,{"a.png","b.png","hidden.png"},"a.png");
    Check(batch==currentTab,"context selection excludes hidden photos");
    Check(Organize::RemoveFromCategory(removalDraft,batch,"daily")==2,"batch removal from current tab");
    Check(removalDraft.labels.at("a.png").categories==std::vector<std::string>({"favorites","tab-last"}) && removalDraft.labels.at("b.png").categories==std::vector<std::string>({"tab-last"}),"other memberships retained after removal");
    Check(Organize::RemoveFromCategory(removalDraft,batch,"daily")==0,"repeated removal is a no-op");
    const auto single=Organize::TrashTargets(currentTab,{"a.png"},"b.png");
    Check(single==std::vector<std::string>({"b.png"}),"unselected context acts only on itself");
    Check(Organize::RemoveFromCategory(removalDraft,single,"tab-last")==1,"remove final membership");
    Check(removalDraft.labels.at("b.png").categories.empty() && !removalDraft.labels.at("b.png").deleted,"last membership removal keeps outfit outside Trash");
    Check(!removalDraft.labels.at("a.png").deleted && removalDraft.labels.at("trash.png").deleted,"Trash flags unchanged");
    Check(removalDraft.order==draft.order && removalDraft.labels.at("a.png").name==draft.labels.at("a.png").name && removalDraft.labels.at("a.png").exchangeSlots==draft.labels.at("a.png").exchangeSlots && removalDraft.labels.at("a.png").outfitItems==draft.labels.at("a.png").outfitItems,"removal preserves order, names and equipment choices");
    for(const auto* invalid:{"","#trash","#head","nonexistent"})Check(Organize::RemoveFromCategory(removalDraft,batch,invalid)==0,"special views are not removable tabs");
    Check(Organize::RemoveFromCategory(removalDraft,{"absent.png","trash.png"},"favorites")==0 && !removalDraft.labels.contains("absent.png"),"unrelated entries not created");
    Check(ReadLibrary(path).labels.at("a.png").categories==draft.labels.at("a.png").categories,"cancelled removal does not change saved memberships");
    WriteLibrary(removalDraft,path);reloaded=ReadLibrary(path);
    Check(reloaded.labels.at("a.png").categories==removalDraft.labels.at("a.png").categories && reloaded.labels.at("b.png").categories.empty() && !reloaded.labels.at("b.png").deleted && reloaded.order==draft.order,"saved removal survives reopening without deleting outfit");
    Check(Organize::AddToCategory(removalDraft,batch,"daily")==2,"removed photos can be registered again");

    // Exercise many selected photos and the last of 64 tabs.
    Library large;std::vector<Preset> many;std::set<std::string> selection;
    for(int i=0;i<64;++i)large.categories.push_back({"tab-"+std::to_string(i),"Tab "+std::to_string(i)});
    for(int i=0;i<250;++i){Preset p;p.photo=std::to_string(i)+".png";many.push_back(p);selection.insert(p.photo);large.order.push_back(p.photo);}
    const auto all=Organize::CategoryTargets(large,many,selection);
    Check(Organize::AddToCategory(large,all,"tab-63")==250,"large selection reaches last tab");
    Check(Organize::AddToCategory(large,all,"tab-63")==0,"large repeated drop does not duplicate");
    std::cout<<"Batch category registration/removal, preservation, cancellation and save/reopen passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
