#include "TransmogFunctionBounds.h"
#include "TransmogCallsites.h"
#include <cassert>
#include <cstring>
#include <vector>
using namespace Gallery;
int main() {
    using FunctionBounds::Entry;
    std::vector<std::uint8_t> memory(0x1000);
    const Entry root{0x100,0x120,0x800},child{0x120,0x150,0x820},grandchild{0x150,0x190,0x840},other{0x190,0x200,0x860};
    auto read=[&](std::uint32_t at,void* out,std::size_t n){if(at>memory.size() || n>memory.size()-at)return false;std::memcpy(out,memory.data()+at,n);return true;};
    auto lookup=[&](std::uint32_t at)->std::optional<Entry>{for(auto e:{root,child,grandchild,other})if(at>=e.begin && at<e.end)return e;return {};};
    memory[0x800]=1;memory[0x860]=1;
    memory[0x820]=0x21;memory[0x822]=3; // odd code count rounds up
    std::memcpy(memory.data()+0x82C,&root,12);
    memory[0x840]=0x21;std::memcpy(memory.data()+0x844,&child,12);
    auto bounds=FunctionBounds::Contiguous(root,read,lookup);
    assert(bounds && bounds->begin==root.begin && bounds->end==grandchild.end);
    // The call in a later chained range must be visible, while a call in the
    // neighboring independent function must not make it ambiguous.
    std::fill(memory.begin()+0x100,memory.begin()+0x200,0x90);
    for(auto at:{0x160u,0x1A0u}) {memory[at]=0xE8;std::int32_t rel=0x500-at-5;std::memcpy(memory.data()+at+1,&rel,4);}
    memory[0x18F]=0xC3;
    auto code=Callsites::Decode(std::span(memory).subspan(bounds->begin,bounds->end-bounds->begin),bounds->begin);
    assert(code && Callsites::UniqueCall(*code,0,[](auto,auto t){return t==0x500;})==0x160);
    assert(!FunctionBounds::Contiguous(child,read,lookup));
    std::memcpy(memory.data()+0x844,&grandchild,12); // self-cycle
    assert(!FunctionBounds::Contiguous(root,read,lookup));
    std::memcpy(memory.data()+0x844,&child,12);
    memory[0x840]=0x29; // illegal handler + chain combination
    assert(!FunctionBounds::Contiguous(root,read,lookup));
    memory[0x840]=0x21;
    const Entry bad{0x150,0x190,0xFFF};
    assert(!FunctionBounds::Root(bad,read));
}
