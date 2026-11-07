#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <cstring>
#include "hde64/hde64.h"
namespace Gallery::Callsites {
struct Instruction {std::uintptr_t at{},call{},lea{},jump{};};
// Decode reachable instructions inside ONE PE unwind function. Never scan raw
// bytes for E8/LEA: immediates, padding and jump tables are not instructions.
inline std::optional<std::vector<Instruction>> Decode(std::span<const std::uint8_t> bytes,std::uintptr_t base,std::size_t* failed=nullptr) {
    if(bytes.empty() || bytes.size()>0x4000) return {};
    std::vector<std::uint8_t> padded(bytes.begin(),bytes.end());padded.resize(bytes.size()+32);
    std::vector<bool> occupied(bytes.size()),starts(bytes.size());
    std::vector<std::size_t> todo{0};std::vector<Instruction> result;
    while(!todo.empty()) {
        auto off=todo.back();todo.pop_back();
        while(off<bytes.size()) {
            if(failed) *failed=off;
            if(starts[off]) break;
            if(occupied[off]) return {};
            hde64s op{};const auto len=hde64_disasm(padded.data()+off,&op);
            if(!len || len>15 || (op.flags&F_ERROR) || off+len>bytes.size()) return {};
            for(std::size_t i=off;i<off+len;++i) {if(occupied[i]) return {};occupied[i]=true;}
            starts[off]=true;
            const auto at=base+off,next=at+len;
            Instruction ins{at};
            if(op.opcode==0xE8 && len==5) ins.call=next+static_cast<std::int32_t>(op.imm.imm32);
            if(op.opcode==0x8D && op.rex_w && op.modrm_mod==0 && op.modrm_rm==5 && !op.p_67)
                ins.lea=next+static_cast<std::int32_t>(op.disp.disp32);
            const bool jump=op.opcode==0xE9 || op.opcode==0xEB;
            const bool conditional=(op.opcode>=0x70 && op.opcode<=0x7F) ||
                (op.opcode==0x0F && op.opcode2>=0x80 && op.opcode2<=0x8F) || (op.opcode>=0xE0 && op.opcode<=0xE3);
            if(jump || conditional) {
                const auto delta=(op.flags&F_IMM8)?static_cast<std::int32_t>(static_cast<std::int8_t>(op.imm.imm8)):static_cast<std::int32_t>(op.imm.imm32);
                const auto target=next+delta;
                if(jump) ins.jump=target;
                if(target>=base && target<base+bytes.size()) todo.push_back(static_cast<std::size_t>(target-base));
            }
            result.push_back(ins);
            if(jump || op.opcode==0xC3 || op.opcode==0xC2 || op.opcode==0xCC ||
               (op.opcode==0xFF && (op.modrm_reg==4 || op.modrm_reg==5))) break;
            off+=len;
        }
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.at<b.at;});
    return result;
}

inline bool HasAnchor(const std::vector<Instruction>& instructions,std::uintptr_t site,std::uintptr_t anchor) {
    return !anchor || std::any_of(instructions.begin(),instructions.end(),[&](const auto& prior){
        return prior.lea==anchor && prior.at<site && site-prior.at<=0x60;
    });
}
// A measured location is accepted only at a reachable decoded CALL boundary.
// This is a fallback for a call whose callee was already replaced by a mod.
inline const Instruction* MeasuredCall(const std::vector<Instruction>& instructions,std::uintptr_t site,std::uintptr_t anchor=0) {
    if(!site || !HasAnchor(instructions,site,anchor)) return nullptr;
    const auto found=std::find_if(instructions.begin(),instructions.end(),[&](const auto& ins){return ins.at==site && ins.call;});
    return found==instructions.end()?nullptr:&*found;
}
template<class Accept>
std::optional<std::uintptr_t> UniqueCall(const std::vector<Instruction>& instructions,std::uintptr_t anchor,Accept accept) {
    std::optional<std::uintptr_t> found;
    for(const auto& ins:instructions) {
        if(!ins.call || !accept(ins.at,ins.call)) continue;
        if(anchor && std::none_of(instructions.begin(),instructions.end(),[&](const auto& prior){return prior.lea==anchor && prior.at<ins.at && ins.at-prior.at<=0x60;})) continue;
        if(found) return {}; // Ambiguous means disabled, never pick the first.
        found=ins.at;
    }
    return found;
}
}
