#include "NativeHairPhysicsPolicy.h"
#include <cassert>
#include <cstddef>
#include <stdexcept>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
using namespace Gallery;
struct Probe {std::uint8_t flag{};const void* forwarded{};bool deactivated{};};
static_assert(offsetof(Probe,forwarded)==8 && offsetof(Probe,deactivated)==16);
using ProbeFn=bool(*)(Probe*,const void*,bool);
ProbeFn probeOriginal{};
bool ProbeHook(Probe* probe,const void* argument,bool deactivate) {
    ScopedHeadPhysics binding(probe->flag,true);
    return probeOriginal(probe,argument,deactivate);
}
void ExecuteContinuationTest() {
    // Execute the exact entry/continuation scheme against a synthetic function
    // in THIS test process. No game/DLL is loaded, patched or required.
    auto* code=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    assert(code);
    std::memcpy(code,NativeHairEntry.data(),NativeHairEntry.size());
    const std::array<std::uint8_t,12> body{
        0x48,0x89,0x51,0x08, // mov [rcx+8],rdx
        0x44,0x88,0x41,0x10, // mov [rcx+10h],r8b
        0x0f,0xb6,0x01,0xc3  // movzx eax,byte [rcx]; ret
    };
    std::memcpy(code+10,body.data(),body.size());
    FlushInstructionCache(GetCurrentProcess(),code,0x1000);
    auto target=reinterpret_cast<ProbeFn>(code);
    Probe probe{};int marker{};
    assert(!target(&probe,&marker,false));
    assert(probe.forwarded==&marker && !probe.deactivated && probe.flag==0);
    const auto bridge=NativeHairContinuation(reinterpret_cast<std::uint64_t>(code+10));
    std::memcpy(code+128,bridge.data(),bridge.size());
    probeOriginal=reinterpret_cast<ProbeFn>(code+128);
    const auto hook=reinterpret_cast<std::uintptr_t>(&ProbeHook);
    std::memcpy(code+64,&hook,sizeof(hook));
    const auto patch=NativeHairEntryPatch(64-6);
    std::memcpy(code,patch.data(),patch.size());
    FlushInstructionCache(GetCurrentProcess(),code,0x1000);
    assert(target(&probe,&marker,true));
    assert(probe.forwarded==&marker && probe.deactivated && probe.flag==0);
    probe.flag=1;assert(target(&probe,&marker,false));
    assert(probe.forwarded==&marker && !probe.deactivated && probe.flag==1);
    probeOriginal=nullptr;assert(VirtualFree(code,0,MEM_RELEASE));
}
int main(){
    constexpr auto body=1u<<2,hair=2u,head=1u,longhair=1u<<11;
    assert(NativeHairPhysicsVisible(~0u,body));
    assert(!NativeHairPhysicsVisible(~0u,body|hair));
    assert(!NativeHairPhysicsVisible(~0u,head));
    assert(!NativeHairPhysicsVisible(body,0));
    assert(!NativeHairPhysicsVisible(longhair,0));
    assert(NativeHairPhysicsVisible(hair,0));
    std::uint8_t flag=0;
    {ScopedHeadPhysics binding(flag,true);assert(flag==1);}
    assert(flag==0);
    {ScopedHeadPhysics binding(flag,false);assert(flag==0);}
    assert(flag==0);
    flag=1;{ScopedHeadPhysics binding(flag,true);assert(flag==1);}assert(flag==1);
    flag=0;try{ScopedHeadPhysics binding(flag,true);assert(flag==1);throw std::runtime_error("stop");}
    catch(const std::runtime_error&){}assert(flag==0);
    flag=3;{ScopedHeadPhysics binding(flag,true);assert(flag==3);}assert(flag==3);
    constexpr auto minimum=std::numeric_limits<std::int32_t>::min();
    constexpr auto maximum=std::numeric_limits<std::int32_t>::max();
    assert(NativeHairNearEnough(minimum) && NativeHairNearEnough(maximum));
    assert(!NativeHairNearEnough(static_cast<std::intptr_t>(minimum)-1));
    assert(!NativeHairNearEnough(static_cast<std::intptr_t>(maximum)+1));
    const std::uint64_t destination=0x123456789abcdef0;
    const auto bridge=NativeHairContinuation(destination);
    for(unsigned i=0;i<10;++i)assert(bridge[i]==NativeHairEntry[i]);
    assert(bridge[10]==0xff && bridge[11]==0x25);
    for(unsigned i=12;i<16;++i)assert(bridge[i]==0);
    std::uint64_t address{};std::memcpy(&address,bridge.data()+16,8);assert(address==destination);
    const auto patch=NativeHairEntryPatch(-123456);
    assert(patch[0]==0xff && patch[1]==0x25);
    std::int32_t displacement{};std::memcpy(&displacement,patch.data()+2,4);assert(displacement==-123456);
    for(unsigned i=6;i<10;++i)assert(patch[i]==0x90);
    ExecuteContinuationTest();
}
