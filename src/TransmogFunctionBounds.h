#pragma once
#include <array>
#include <cstdint>
#include <optional>
namespace Gallery::FunctionBounds {
struct Entry {
    std::uint32_t begin{},end{},unwind{};
    bool operator==(const Entry&) const = default;
};
static_assert(sizeof(Entry)==12);
// Windows x64 UNWIND_INFO: chained entries describe multiple ranges of ONE
// function. Never assume the adjacent pdata entry is part of that function.
template<class Read>
std::optional<Entry> Root(Entry entry,Read read) {
    std::array<std::uint32_t,32> visited{};
    for(unsigned depth=0;depth<visited.size();++depth) {
        if(entry.begin>=entry.end || !entry.unwind || (entry.unwind&3)) return {};
        for(unsigned i=0;i<depth;++i) if(visited[i]==entry.unwind) return {};
        visited[depth]=entry.unwind;
        std::array<std::uint8_t,4> header{};
        if(!read(entry.unwind,header.data(),header.size()) || (header[0]&7)!=1) return {};
        const auto flags=header[0]>>3;
        if(!(flags&4)) return entry;
        if(flags&3) return {}; // CHAININFO and handler flags are mutually exclusive.
        const auto alignedCodes=(static_cast<std::uint32_t>(header[2])+1)&~1u;
        const auto offset=4+2*alignedCodes;
        if(entry.unwind>UINT32_MAX-offset) return {};
        if(!read(entry.unwind+offset,&entry,sizeof(entry))) return {};
    }
    return {};
}
template<class Read,class Lookup>
std::optional<Entry> Contiguous(Entry first,Read read,Lookup lookup) {
    const auto root=Root(first,read);
    if(!root || *root!=first || first.end-first.begin>0x4000) return {};
    auto span=first;
    for(unsigned count=0;count<128;++count) {
        const auto next=lookup(span.end);
        if(!next || next->begin!=span.end) return span;
        const auto nextRoot=Root(*next,read);
        if(!nextRoot) return {};
        if(*nextRoot!=first) return span;
        if(next->end<=span.end || next->end-first.begin>0x4000) return {};
        span.end=next->end;
    }
    return {};
}
}
