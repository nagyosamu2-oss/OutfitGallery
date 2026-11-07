#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include "hde64/hde64.h"
namespace Gallery::HookDiagnostics {
// Diagnostic only: never used to approve a patch. Follow bounded plain jumps,
// then inspect decoded RIP-relative calls in a generated block. Do not scan
// neighboring trampoline allocations or interpret arbitrary bytes as pointers.
template<class Read,class Module>
std::string RelatedModule(std::uintptr_t cursor,Read read,Module module) {
    std::array<std::uintptr_t,8> seen{};
    for(unsigned hop=0;hop<seen.size();++hop) {
        for(unsigned i=0;i<hop;++i) if(seen[i]==cursor) return {};
        seen[hop]=cursor;
        if(auto name=module(cursor);!name.empty()) return name;
        std::array<std::uint8_t,6> stub{};
        if(!read(cursor,stub.data(),stub.size())) return {};
        std::int32_t displacement{};
        if(stub[0]==0xE9) {
            std::memcpy(&displacement,stub.data()+1,4);cursor+=5+displacement;
        } else if(stub[0]==0xFF && stub[1]==0x25) {
            std::memcpy(&displacement,stub.data()+2,4);
            if(!read(cursor+6+displacement,&cursor,sizeof(cursor))) return {};
        } else {
            for(std::size_t offset=0;offset<128;) {
                std::array<std::uint8_t,32> bytes{};
                if(!read(cursor+offset,bytes.data(),15)) return {};
                hde64s op{};const auto len=hde64_disasm(bytes.data(),&op);
                if(!len || (op.flags&F_ERROR) || offset+len>128) return {};
                if(op.opcode==0xFF && op.modrm_reg==2 && op.modrm_mod==0 && op.modrm_rm==5 && !op.p_67) {
                    std::uintptr_t target{};
                    if(read(cursor+offset+len+static_cast<std::int32_t>(op.disp.disp32),&target,sizeof(target)))
                        if(auto name=module(target);!name.empty()) return name;
                }
                if(op.opcode==0xC3 || op.opcode==0xC2 || op.opcode==0xCC || op.opcode==0xE9 || op.opcode==0xEB ||
                   (op.opcode==0xFF && (op.modrm_reg==4 || op.modrm_reg==5))) return {};
                offset+=len;
            }
            return {};
        }
    }
    return {};
}
}
