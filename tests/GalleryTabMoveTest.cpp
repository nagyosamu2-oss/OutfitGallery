#include "GalleryOrganize.h"
#include <cassert>
int main() {
    using namespace Gallery;
    Library lib;
    lib.categories={{"from","Source"},{"to","Destination"},{"other","Other"},{"acc","Accessories",true}};
    lib.order={"a","b","hidden","trash"};
    lib.labels["a"].categories={"from","other"};
    lib.labels["b"].categories={"from","to"};
    lib.labels["hidden"].categories={"from"};
    lib.labels["trash"].categories={"from"};lib.labels["trash"].deleted=true;
    lib.variations={{"a","b"}};
    const auto order=lib.order;const auto groups=lib.variations;
    assert(Organize::MoveToCategory(lib,{"a","b","a","trash","missing"},"from","to")==2);
    assert((lib.labels["a"].categories==std::vector<std::string>{"other","to"}));
    assert((lib.labels["b"].categories==std::vector<std::string>{"to"}));
    assert((lib.labels["hidden"].categories==std::vector<std::string>{"from"}));
    assert(lib.labels["trash"].deleted && lib.labels.size()==4 && lib.order==order && lib.variations==groups);
    assert(Organize::MoveToCategory(lib,{"hidden"},"from","from")==0);
    assert(Organize::MoveToCategory(lib,{"hidden"},"from","acc")==0);
    assert(Organize::MoveToCategory(lib,{"hidden"},"","to")==0);
    assert(Organize::MoveToCategory(lib,{"hidden"},"#trash","to")==0);
    assert(Organize::MoveToCategory(lib,{"hidden"},"from","missing")==0);
    // Cancelled draft and ordinary registration preserve the committed source.
    auto draft=lib;
    assert(Organize::AddToCategory(draft,{"hidden"},"to")==1);
    assert(draft.labels["hidden"].categories.size()==2);
    assert(lib.labels["hidden"].categories.size()==1);
    assert(Organize::MoveToCategory(draft,{"hidden"},"from","to")==1);
    assert(draft.labels["hidden"].categories.size()==1);
}
