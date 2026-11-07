#include "TransmogCallsites.h"
#include "TransmogRuntime.h"
#include <cassert>
using namespace Gallery;
int main() {
    constexpr std::uintptr_t base=0x1000,callee=0x2000,anchor=0x3000;
    auto emit=[](auto& b,std::size_t at,std::uint8_t opcode,std::uintptr_t to) {
        b[at]=opcode;const auto d=static_cast<std::int32_t>(to-(base+at+5));std::memcpy(b.data()+at+1,&d,4);
    };
    std::vector<std::uint8_t> bytes(64,0x90);
    // A fake E8 inside mov rax, imm64 must never be found as a call.
    bytes[0]=0x48;bytes[1]=0xB8;emit(bytes,2,0xE8,callee);
    bytes[10]=0x48;bytes[11]=0x8D;bytes[12]=0x05;
    const std::int32_t lea=static_cast<std::int32_t>(anchor-(base+17));std::memcpy(bytes.data()+13,&lea,4);
    emit(bytes,20,0xE8,callee);bytes[25]=0xC3;
    emit(bytes,30,0xE8,callee); // unreachable bytes after return
    auto decoded=Callsites::Decode(bytes,base);assert(decoded);
    auto accept=[&](auto,auto target){return target==callee;};
    assert(Callsites::UniqueCall(*decoded,anchor,accept)==base+20);
    assert(!Callsites::UniqueCall(*decoded,anchor+1,accept));
    bytes[25]=0x90;bytes[35]=0xC3;decoded=Callsites::Decode(bytes,base);assert(decoded);
    assert(!Callsites::UniqueCall(*decoded,anchor,accept)); // two matches must fail
    // Branch into the middle of an instruction is rejected.
    assert(!Callsites::Decode(std::array<std::uint8_t,4>{0x75,0xFF,0x90,0xC3},base));
    assert(!Callsites::Decode(std::array<std::uint8_t,2>{0xE8,0},base));
    // Forward jump bypasses fake call bytes; negative displacement is decoded.
    decoded=Callsites::Decode(std::array<std::uint8_t,9>{0xEB,0x05,0xE8,0,0,0,0,0xC3,0},base);
    assert(decoded && !Callsites::UniqueCall(*decoded,0,accept));
    assert(TransmogRuntimes.size()==7);
    assert((TransmogRuntimes.back()==std::array<std::uint16_t,4>{1,6,1179,0}));
}
