#include "GalleryOrganize.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try {
    Library library;
    library.categories={{"daily","Daily"},{"gloves","Gloves",true},{"shoes","Shoes",true}};
    library.labels["gloves.png"].categories={"gloves"};
    library.order={"gloves.png","full.png","head.png"};
    Preset accessory;accessory.photo="gloves.png";accessory.accessories=true;accessory.slotMask=8;
    accessory.items.push_back({"Gloves.esp",1,"Gloves","armor"});
    Preset outfit;outfit.photo="full.png";
    Preset head;head.photo="head.png";head.slotMask=2;
    Check(CollectionFits(8,"gloves",true,library),"accessory fits its custom tab");
    Check(CollectionFits(0,"gloves",true,library),"slotless accessory preset keeps its type");
    Check(!CollectionFits(0,"gloves",false,library),"full outfit cannot enter accessory view");
    Check(!CollectionFits(2,"gloves",false,library),"head preset does not become accessory");
    Check(!CollectionFits(8,"daily",true,library),"accessory stays out of full outfit tabs");
    Check(CollectionFits(0,"daily",false,library),"existing tab still accepts full outfit");
    Check(CollectionFits(8,"#accessories",true,library),"default collection remains available");
    Check(!CollectionFits(8,"",true,library),"full All view stays unchanged");
    Check(ResolveStartupTab("gloves",library)=="gloves","accessory tab supports startup selection");
    library.categories.push_back({"later-full","Later full outfit tab"});
    const auto tabs=CollectionTabs(library);
    std::vector<std::string> tabIDs;for(const auto& c:tabs)tabIDs.push_back(c.id);
    Check(tabIDs==std::vector<std::string>({"","daily","later-full","#head","#accessories","gloves","shoes","#trash"}),"custom accessories appear immediately after default accessories");
    Check(library.categories[1].id=="gloves" && library.categories.back().id=="later-full","display order does not modify saved library");
    Check(NextAccessoryTab("daily",library)=="#accessories","R2 enters default accessories from another category");
    Check(NextAccessoryTab("#head",library)=="#accessories","R2 enters default accessories from head category");
    Check(NextAccessoryTab("#accessories",library)=="gloves","R2 advances to first accessory tab");
    Check(NextAccessoryTab("gloves",library)=="shoes","R2 advances in display order");
    Check(NextAccessoryTab("shoes",library)=="#accessories","R2 wraps to default accessories");
    Check(NextAccessoryTab("removed-tab",library)=="#accessories","removed category safely returns to default");
    Check(NextAccessoryTab("#accessories",Library{})=="#accessories","no custom accessories retains original R2 shortcut");
    Check(CollectionTabs(library,true)[tabs.size()-1].id=="#legacy","legacy view follows accessory tabs");
    library.categories.pop_back();
    const std::vector<Preset> visible{accessory,outfit,head};
    const std::set<std::string> selected{"gloves.png","full.png","head.png","hidden.png"};
    const auto accessories=Organize::CategoryTargets(library,visible,selected,true);
    const auto outfits=Organize::CategoryTargets(library,visible,selected);
    Check(accessories==std::vector<std::string>{"gloves.png"},"batch separates accessory scope");
    Check(outfits==std::vector<std::string>{"full.png"},"batch preserves full outfit scope");
    const auto before=library;
    Check(Organize::AddToCategory(library,accessories,"shoes")==1,"accessory can have multiple memberships");
    Check(Organize::AddToCategory(library,accessories,"shoes")==0,"repeated registration is idempotent");
    Check(library.labels.at("gloves.png").categories==std::vector<std::string>({"gloves","shoes"}),"original membership preserved");
    Check(accessory.accessories && accessory.slotMask==8 && accessory.items.size()==1,"classification does not convert equipment scope");
    Check(before.labels.at("gloves.png").categories.size()==1,"cancelled draft remains untouched");

    const auto folder=std::filesystem::temp_directory_path()/"outfit-gallery-accessory-category-test";
    std::filesystem::create_directories(folder);
    const auto path=folder/"Library.json";
    WriteLibrary(library,path);
    auto saved=ReadLibrary(path);
    Check(saved.categories.size()==3 && !saved.categories[0].accessories && saved.categories[1].accessories && saved.categories[2].accessories,"tab types survive save and reopen");
    Check(saved.labels.at("gloves.png").categories==library.labels.at("gloves.png").categories,"memberships survive reopening");
    Check(Organize::RemoveFromCategory(saved,accessories,"gloves")==1,"remove membership only");
    Check(!saved.labels.at("gloves.png").deleted && saved.labels.at("gloves.png").categories==std::vector<std::string>{"shoes"} && saved.order==library.order,"removal retains photo and order");
    saved.categories[2].name="Footwear";WriteLibrary(saved,path);
    Check(ReadLibrary(path).categories[2].accessories,"renaming retains accessory type");
    {std::ofstream out(folder/"legacy.json");out<<R"({"schema":1,"categories":[{"id":"old","name":"Old tab"}],"labels":{}})";}
    Check(!ReadLibrary(folder/"legacy.json").categories[0].accessories,"legacy library defaults to full outfit categories");
    {std::ofstream out(folder/"invalid.json");out<<R"({"schema":1,"categories":[{"id":"bad","name":"Bad tab","accessories":"true"}],"labels":{}})";}
    bool refused=false;try{ReadLibrary(folder/"invalid.json");}catch(...){refused=true;}
    Check(refused,"invalid tab type is rejected");
    std::cout<<"Accessory tab scope, membership, legacy loading and persistence passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
