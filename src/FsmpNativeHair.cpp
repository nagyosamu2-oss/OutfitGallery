#include "PCH.h"
#include "FsmpNativeHair.h"
#include "NativeHairPhysicsPolicy.h"
#include <atomic>
#include <bcrypt.h>
#include <windows.h>
namespace Gallery {
namespace {
// EXPERIMENT: exactly the installed FSMP4.1.1 AE AVX build, measured from its
// matching PDB and DLL. These are FOREIGN-module RVAs, never game addresses.
// updateAttachedState runs under ActorManager's recursive mutex. It consumes
// head.isActive while preserving FSMP's own activity/distance/budget gates.
constexpr std::uint32_t Stamp=0x6a8f44a9,ImageSize=0x41b000,UpdateRva=0x92790;
constexpr std::size_t UpdateSize=0x372,HeadFlag=0xf1,HeadNode=0x40;
constexpr std::array<std::uint8_t,32> UpdateHash{
    0xea,0x61,0xd3,0x00,0xd9,0x96,0x16,0xb9,0x13,0x7a,0x32,0x22,0x7c,0x33,0xbe,0x77,
    0xa7,0x40,0xf9,0xe1,0xbe,0x02,0xb6,0x25,0x89,0x3d,0xf5,0x4f,0x1b,0xb8,0xa8,0x1f};
using UpdateFn=bool(*)(void*,const RE::NiNode*,bool);
std::atomic<UpdateFn> original{};
std::atomic<NativeHairDecision> provider{};
std::atomic<bool> wasForced{};
void* bridgePage{}; // process-lifetime executable continuation; no unload support

bool HashMatches(const void* bytes,std::size_t size) {
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return false;
    BCRYPT_HASH_HANDLE hash{};
    bool matched=false;
    if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::array<std::uint8_t,32> digest{};
        if(BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),static_cast<ULONG>(size),0)>=0 &&
           BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0) matched=digest==UpdateHash;
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(algorithm,0);
    return matched;
}
void* AllocateNear(std::uintptr_t target) {
    SYSTEM_INFO system{};GetSystemInfo(&system);
    const auto granularity=static_cast<std::uintptr_t>(system.dwAllocationGranularity);
    const auto aligned=target&~(granularity-1);
    // Optional integration: a failed near allocation must not abort the game.
    for(std::uintptr_t distance=granularity;distance<0x7ffe0000;distance+=granularity) {
        for(const bool above:{true,false}) {
            if(!above && aligned<distance) continue;
            const auto candidate=above?aligned+distance:aligned-distance;
            MEMORY_BASIC_INFORMATION info{};
            if(!VirtualQuery(reinterpret_cast<void*>(candidate),&info,sizeof(info)) || info.State!=MEM_FREE) continue;
            if(auto* memory=VirtualAlloc(reinterpret_cast<void*>(candidate),0x1000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)) return memory;
        }
    }
    return nullptr;
}
bool Update(void* skeleton,const RE::NiNode* playerCell,bool deactivate) {
    const auto run=original.load();
    if(!skeleton || !run) return false;
    auto* bytes=static_cast<std::byte*>(skeleton);
    // Both pointers are held by this FSMP Skeleton for the duration of its own
    // locked update. No scene pointer or Skeleton pointer survives this call.
    RE::TESObjectREFR* owner{};void* head{};
    std::memcpy(&owner,bytes,sizeof(owner));
    std::memcpy(&head,bytes+HeadNode,sizeof(head));
    auto& flag=*reinterpret_cast<std::uint8_t*>(bytes+HeadFlag);
    const auto decide=provider.load();
    const bool player=owner && owner->GetFormID()==0x14;
    const bool force=player && head && flag==0 && decide && decide(0x14);
    if(player && wasForced.exchange(force)!=force)
        SKSE::log::info("FSMP-NATIVE1 player native-hair override={}; original FSMP flag retained after update",force);
    // Leave every other actor on the exact original path, with no flag store.
    if(!force) return run(skeleton,playerCell,deactivate);
    ScopedHeadPhysics useNative(flag,true);
    return run(skeleton,playerCell,deactivate);
}
}
void InstallFsmpNativeHair(NativeHairDecision decision) {
    if(original.load()) return;
    // The first prototype is measured on this game/runtime + this FSMP binary.
    if(REL::Module::get().version()!=REL::Version(1,6,1170,0)) {
        SKSE::log::info("FSMP-NATIVE1 inactive: this build supports game1.6.1170 only");return;
    }
    const auto module=GetModuleHandleW(L"hdtsmp64.dll");
    if(!module) {SKSE::log::info("FSMP-NATIVE1 inactive: hdtsmp64.dll not loaded");return;}
    const auto base=reinterpret_cast<std::uintptr_t>(module);
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || dos->e_lfanew>0x1000) return;
    const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       pe->FileHeader.TimeDateStamp!=Stamp || pe->OptionalHeader.SizeOfImage!=ImageSize) {
        SKSE::log::info("FSMP-NATIVE1 inactive: unmeasured FSMP build; existing settings and behavior preserved");return;
    }
    const auto target=base+UpdateRva;
    if(!HashMatches(reinterpret_cast<void*>(target),UpdateSize) ||
       std::memcmp(reinterpret_cast<void*>(target),NativeHairEntry.data(),NativeHairEntry.size())!=0) {
        SKSE::log::warn("FSMP-NATIVE1 inactive: FSMP update code differs from measured4.1.1 AVX build");return;
    }
    auto* memory=static_cast<std::byte*>(AllocateNear(target));
    if(!memory) {SKSE::log::warn("FSMP-NATIVE1 inactive: near-code allocation unavailable");return;}
    const auto continuation=NativeHairContinuation(target+NativeHairEntry.size());
    std::memcpy(memory,continuation.data(),continuation.size());
    const auto destination=reinterpret_cast<std::uintptr_t>(&Update);
    std::memcpy(memory+32,&destination,sizeof(destination));
    const auto distance=reinterpret_cast<std::intptr_t>(memory+32)-static_cast<std::intptr_t>(target+6);
    DWORD previous{};
    if(!NativeHairNearEnough(distance) || !VirtualProtect(memory,0x1000,PAGE_EXECUTE_READ,&previous)) {
        VirtualFree(memory,0,MEM_RELEASE);SKSE::log::warn("FSMP-NATIVE1 inactive: continuation preparation failed");return;
    }
    FlushInstructionCache(GetCurrentProcess(),memory,0x1000);
    const auto patch=NativeHairEntryPatch(static_cast<std::int32_t>(distance));
    provider.store(decision);
    original.store(reinterpret_cast<UpdateFn>(memory));
    if(!REL::safe_write(target,patch.data(),patch.size(),NativeHairEntry.data(),NativeHairEntry.size())) {
        original.store(nullptr);provider.store(nullptr);VirtualFree(memory,0,MEM_RELEASE);
        SKSE::log::warn("FSMP-NATIVE1 inactive: entry changed before installation");return;
    }
    bridgePage=memory;
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(target),patch.size());
    SKSE::log::info("FSMP-NATIVE1 installed: FSMP4.1.1 AE AVX; player only; scoped native-hair decision; config/inventory unchanged");
}
}
