#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace Gallery::HookChain {
enum class Owner { None, IED, Other };
struct Target { std::uintptr_t direct{}, resolved{}; bool ied{}, foreign{}; };
// Reads are supplied by the caller (ReadProcessMemory in-game). Only accept an
// E8 call followed by at most three plain jump stubs. Never scan arbitrary code.
template<class Read, class Module, class Executable>
Target Validate(std::uintptr_t site, std::uintptr_t vanilla, Read read, Module module, Executable executable, bool allowForeign=true) {
    std::array<unsigned char,5> call{};
    if(!read(site,call.data(),call.size()) || call[0]!=0xE8) return {};
    std::int32_t delta{};
    std::memcpy(&delta,call.data()+1,4);
    const auto direct=site+5+delta;
    if(direct==vanilla) return executable(direct)?Target{direct,direct,false,false}:Target{};
    auto cursor=direct;
    std::array<std::uintptr_t,4> visited{};
    for(unsigned hop=0;hop<visited.size();++hop) {
        for(unsigned i=0;i<hop;++i) if(visited[i]==cursor) return {};
        visited[hop]=cursor;
        if(!cursor || !executable(cursor)) return {};
        const auto owner=module(cursor);
        if(owner==Owner::IED) return {direct,cursor,true,false};
        // Only used after the engine call SITE has been identified separately.
        // Preserve the direct callee, including other DLLs' trampolines.
        if(owner==Owner::Other) return allowForeign?Target{direct,cursor,false,true}:Target{};
        if(hop==3) return {};
        std::array<unsigned char,6> stub{};
        if(!read(cursor,stub.data(),stub.size())) return {};
        if(stub[0]==0xFF && stub[1]==0x25) {
            std::memcpy(&delta,stub.data()+2,4);
            std::uintptr_t next{};
            if(!read(cursor+6+delta,&next,sizeof(next))) return {};
            cursor=next;
        } else if(stub[0]==0xE9) {
            std::memcpy(&delta,stub.data()+1,4);
            cursor=cursor+5+delta;
        } else return {};
    }
    return {};
}
}
