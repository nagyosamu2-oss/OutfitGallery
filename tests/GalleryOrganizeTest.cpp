#include "GalleryOrganize.h"
#include <iostream>
#include <stdexcept>
using namespace Gallery::Organize;
void Check(bool ok){if(!ok)throw std::runtime_error("Organizer test failed");}
int main(){try {
    const std::vector<std::string> original{"a","hidden1","b","c","hidden2","d","e"};
    const std::vector<std::string> view{"a","b","c","d","e"};
    Check(TrashTargets(view,{"b","d"},"b")==std::vector<std::string>({"b","d"}));
    Check(TrashTargets(view,{"b","d"},"c")==std::vector<std::string>({"c"}));
    Check(TrashTargets(view,{"b","hidden1"})==std::vector<std::string>({"b"}));
    Check(TrashTargets(view,{},"b")==std::vector<std::string>({"b"}));
    Check(TrashTargets(view,{}).empty());
    Check(TrashTargets(view,{"b"},"missing").empty());
    auto order=original;
    Check(Move(order,view,{"b","d"},{},false));
    Check(order==std::vector<std::string>({"b","hidden1","d","a","hidden2","c","e"}));
    order=original;Check(Move(order,view,{"b","d"},{},true));
    Check(order==std::vector<std::string>({"a","hidden1","c","e","hidden2","b","d"}));
    order=original;Check(Move(order,view,{"b","d"},"c",true));
    Check(order==std::vector<std::string>({"a","hidden1","c","b","hidden2","d","e"}));
    order=original;Check(Move(order,view,{"a","e"},"c",false));
    Check(order==std::vector<std::string>({"b","hidden1","a","e","hidden2","c","d"}));
    const auto before=order;Check(!Move(order,view,{"a"},"a"));Check(order==before);
    Check(!Move(order,view,{"a"},"missing"));Check(order==before);
    Check(!Move(order,{"a","a"},{"a"}));Check(order==before);
    Check(!Move(order,{"a","not-present"},{"a"}));Check(order==before);
    Check(!Move(order,view,{}));Check(order==before);
    order=original;Check(!Move(order,view,{"a","b","c","d","e"}));Check(order==original);
    Check(!Move(order,view,{"hidden1"}));Check(order==original);
    // Current order is authoritative, not selection click order or display sorting.
    Check(Move(order,{"e","d","c","b","a"},{"d","b"},{},true));
    Check(order[5]=="b" && order[6]=="d");
    std::set<std::string> selected;SelectRange(selected,view,"d","b");Check(selected==std::set<std::string>({"b","c","d"}));
    SelectRange(selected,view,"gone","e");Check(selected.contains("e"));
    // A draft is independent of the committed order: cancellation leaves it untouched.
    const auto committed=original;auto draft=committed;Move(draft,view,{"e"});Check(committed==original && draft!=committed);
    // Large multi-selection preserves every hidden slot and the full permutation.
    std::vector<std::string> large,subset;std::set<std::string> chosen;
    for(int i=0;i<10000;++i){auto id=std::to_string(i);large.push_back(id);if(i%2==0)subset.push_back(id);if(i%6==0)chosen.insert(id);}
    auto largeBefore=large;Check(Move(large,subset,chosen,{},true));
    for(int i=1;i<10000;i+=2)Check(large[i]==largeBefore[i]);
    Check(std::set<std::string>(large.begin(),large.end())==std::set<std::string>(largeBefore.begin(),largeBefore.end()));
    // Fixed grid destination remains the same index regardless of source side.
    const std::vector<std::string> grid{"0","1","2","3","4","5","6","7"};
    auto fixed=grid;Check(Move(fixed,grid,{"0","1"},"4",false,true));
    Check(fixed==std::vector<std::string>({"2","3","4","5","0","1","6","7"}));
    fixed=grid;Check(Move(fixed,grid,{"6","7"},"2",false,true));
    Check(fixed[2]=="6" && fixed[3]=="7");
    fixed=grid;Check(Move(fixed,grid,{"0","7"},"4",false,true));
    Check(fixed[4]=="0" && fixed[5]=="7");
    fixed=grid;Check(Move(fixed,grid,{"0","1"},"7",false,true));
    Check(fixed[6]=="0" && fixed[7]=="1"); // End clamps to fit the entire group.
    fixed=original;Check(Move(fixed,view,{"a","b"},"c",false,true));
    Check(fixed[3]=="a" && fixed[5]=="b" && fixed[1]=="hidden1" && fixed[4]=="hidden2");
    for(const auto chosen: {std::set<std::string>{"0","1"},std::set<std::string>{"6","7"},std::set<std::string>{"0","7"}}) {
        for(const auto& target:grid)if(!chosen.contains(target)) {
            auto result=grid;Move(result,grid,chosen,target,false);
            auto first=std::find(result.begin(),result.end(),target);
            for(const auto& id:chosen)Check(std::find(result.begin(),result.end(),id)<first);
            std::vector<std::string> beforeRemaining,afterRemaining;
            for(const auto& id:grid)if(!chosen.contains(id))beforeRemaining.push_back(id);
            for(const auto& id:result)if(!chosen.contains(id))afterRemaining.push_back(id);
            Check(beforeRemaining==afterRemaining);
        }
    }
    std::cout<<"Organizer subset, batch order, invalid target, range and draft checks passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
