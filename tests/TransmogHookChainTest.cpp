#include "TransmogHookChain.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace Gallery::HookChain;
void Require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    std::vector<unsigned char> memory(4096);
    auto read=[&](std::uintptr_t p,void* out,std::size_t n) {
        if(p>memory.size() || n>memory.size()-p) return false;
        std::memcpy(out,memory.data()+p,n);return true;
    };
    auto owner=[](std::uintptr_t p) {return p==3000?Owner::IED:p>=2000?Owner::Other:Owner::None;};
    bool executable=true;
    auto execute=[&](std::uintptr_t){return executable;};
    auto jump=[&](std::uintptr_t p,std::uintptr_t to,unsigned char opcode) {
        memory[p]=opcode;auto delta=static_cast<std::int32_t>(to-p-5);
        std::memcpy(memory.data()+p+1,&delta,4);
    };
    auto validate=[&] {return Validate(100,2500,read,owner,execute);};
    jump(100,2500,0xE8);
    Require(validate().direct==2500 && !validate().ied,"vanilla accepted");
    jump(100,3000,0xE8);
    Require(validate().ied,"direct IED accepted");
    jump(100,500,0xE8);
    memory[500]=0xFF;memory[501]=0x25;
    std::int32_t offset=94;std::memcpy(memory.data()+502,&offset,4);
    std::uintptr_t target=3000;std::memcpy(memory.data()+600,&target,8);
    Require(validate().ied && validate().direct==500 && validate().resolved==3000,"FF25 preserves trampoline as callee");
    jump(500,700,0xE9);jump(700,900,0xE9);jump(900,3000,0xE9);
    Require(validate().ied,"three jump chain");
    jump(900,1100,0xE9);jump(1100,3000,0xE9);
    Require(!validate().direct,"excessive hops refused");
    jump(700,500,0xE9);Require(!validate().direct,"cycle refused");
    jump(500,2100,0xE9);
    Require(validate().direct==500 && validate().resolved==2100 && validate().foreign,"foreign ordinary function chained without skipping trampoline");
    Require(!Validate(100,2500,read,owner,execute,false).direct,"unknown callee is not enough to discover an unmeasured site");
    jump(500,3000,0xE9);executable=false;Require(!validate().direct,"non-executable refused");executable=true;
    memory[500]=0xC3;Require(!validate().direct,"unknown trampoline refused");
    jump(100,4094,0xE8);
    auto noOwner=[](std::uintptr_t){return Owner::None;};
    Require(!Validate(100,2500,read,noOwner,execute).direct,"unreadable stub refused");
    jump(100,500,0xE8);memory[500]=0xFF;memory[501]=0x25;
    offset=5000;std::memcpy(memory.data()+502,&offset,4);
    Require(!validate().direct,"unreadable indirect pointer refused");
    jump(100,3000,0xE9);Require(!validate().direct,"non-call site refused");
    std::cout<<"Hook chain validation passed\n";
}
