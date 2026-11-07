#include "PCH.h"
#include "TransmogRuntime.h"
#include "TransmogCallsites.h"
#include "TransmogHookChain.h"
#include "TransmogFunctionBounds.h"
#include "TransmogHookDiagnostics.h"
#include <map>
#include <windows.h>
namespace Gallery {
namespace {
bool Read(std::uintptr_t at,void* out,std::size_t size) {
    SIZE_T read{};return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(at),out,size,&read) && read==size;
}
HookChain::Owner Owner(std::uintptr_t at) {
    HMODULE module{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(at),&module)) return HookChain::Owner::None;
    return module==GetModuleHandleW(L"ImmersiveEquipmentDisplays.dll")?HookChain::Owner::IED:HookChain::Owner::Other;
}
bool Executable(std::uintptr_t at) {
    MEMORY_BASIC_INFORMATION info{};
    if(!VirtualQuery(reinterpret_cast<const void*>(at),&info,sizeof(info)) || info.State!=MEM_COMMIT || (info.Protect&PAGE_GUARD)) return false;
    const auto p=info.Protect&0xFF;return p==PAGE_EXECUTE || p==PAGE_EXECUTE_READ || p==PAGE_EXECUTE_READWRITE || p==PAGE_EXECUTE_WRITECOPY;
}
std::string ModuleName(std::uintptr_t at,bool externalOnly=false) {
    HMODULE module{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(at),&module)) return {};
    if(externalOnly && reinterpret_cast<std::uintptr_t>(module)==REL::Module::get().base()) return {};
    wchar_t path[MAX_PATH]{};
    if(!GetModuleFileNameW(module,path,MAX_PATH)) return {};
    return std::filesystem::path(path).filename().string();
}
TransmogFailure MissingCall(const std::vector<Callsites::Instruction>& code,bool hair) {
    for(const auto& ins:code) {
        if(!ins.jump || std::any_of(code.begin(),code.end(),[&](const auto& next){return next.at==ins.jump;})) continue;
        const auto module=HookDiagnostics::RelatedModule(ins.jump,Read,[](auto at){return ModuleName(at,true);});
        return {TransmogFailureCode::Jump,hair,module,std::format("{} path has external jump at RVA={:#x}, target={:#x}",hair?"hair":"worn",ins.at-REL::Module::get().base(),ins.jump)};
    }
    return {TransmogFailureCode::Callsite,hair,{},hair?"Hair call not uniquely identifiable":"Anchored worn call not uniquely identifiable"};
}
std::optional<std::vector<Callsites::Instruction>> Function(std::uintptr_t at) {
    DWORD64 image{};const auto* fn=RtlLookupFunctionEntry(at,&image,nullptr);
    if(!fn || image!=REL::Module::get().base() || at!=image+fn->BeginAddress || fn->EndAddress<=fn->BeginAddress || fn->EndAddress-fn->BeginAddress>0x4000) return {};
    const auto span=FunctionBounds::Contiguous(
        FunctionBounds::Entry{fn->BeginAddress,fn->EndAddress,fn->UnwindData},
        [&](std::uint32_t rva,void* out,std::size_t size){return Read(image+rva,out,size);},
        [&](std::uint32_t rva)->std::optional<FunctionBounds::Entry> {
            DWORD64 owner{};const auto* next=RtlLookupFunctionEntry(image+rva,&owner,nullptr);
            if(!next || owner!=image) return {};
            return FunctionBounds::Entry{next->BeginAddress,next->EndAddress,next->UnwindData};
        });
    if(!span) {SKSE::log::warn("RUNTIME4 invalid chained unwind metadata at RVA={:#x}",at-image);return {};}
    SKSE::log::info("RUNTIME4 function RVA={:#x} firstEnd={:#x} chainedEnd={:#x}",span->begin,fn->EndAddress,span->end);
    std::vector<std::uint8_t> code(span->end-span->begin);
    if(!Read(at,code.data(),code.size())) return {};
    std::size_t failed{};
    auto decoded=Callsites::Decode(code,at,&failed);
    if(!decoded) SKSE::log::warn("RUNTIME4 decode stopped at RVA={:#x}",at+failed-image);
    return decoded;
}
}
std::optional<TransmogRuntimeSites> LocateTransmogRuntime(TransmogFailure& failure) {
    const auto fail=[&](TransmogFailure error)->std::optional<TransmogRuntimeSites> {
        failure=std::move(error);
        SKSE::log::warn("RUNTIME4 transmog unavailable: {}; {}",DescribeTransmogFailure(failure,false),failure.detail);
        return {};
    };
    try {
        const auto version=REL::Module::get().version();
        const bool allowed=std::any_of(TransmogRuntimes.begin(),TransmogRuntimes.end(),[&](const auto& v){return version==REL::Version(v[0],v[1],v[2],v[3]);});
        if(!allowed) return fail({TransmogFailureCode::Runtime,false,{},version.string()});
        const bool se=version==REL::Version(1,5,97,0);
        const auto choose=[&](std::uint64_t a,std::uint64_t b){return se?a:b;};
        // SE/AE pairs pinned in Fitting Room REAugments and VersionCheck;
        // VisitWornItems additionally matches CommonLib's InventoryChanges.
        const std::array<std::uint64_t,9> ids{choose(24221,24725),choose(24220,24724),choose(15856,16096),choose(15806,16044),
            choose(241890,195851),choose(17392,17792),choose(38867,39907),choose(38404,39395),choose(24231,24735)};
        std::map<std::uint64_t,std::uintptr_t> addresses;
        const REL::Offset2ID db{};
        const auto base=REL::Module::get().base();
        for(const auto& entry:db) if(std::find(ids.begin(),ids.end(),entry.id)!=ids.end()) addresses[entry.id]=base+entry.offset;
        for(unsigned i=0;i<8;++i) if(!addresses.contains(ids[i])) return fail({TransmogFailureCode::Address,false,{},std::format("runtime={} missing Address Library ID={}",version.string(),ids[i])});
        TransmogRuntimeSites sites;
        sites.visitTarget=addresses.at(ids[2]);sites.maskTarget=addresses.at(ids[3]);sites.visitorTable=addresses.at(ids[4]);
        sites.stage=addresses.at(ids[5]);sites.refreshFlag=addresses.at(ids[6]);sites.refreshModels=addresses.at(ids[7]);
        const auto text=REL::Module::get().segment(REL::Segment::textx);
        auto inText=[&](std::uintptr_t at){return at>=text.address() && at<text.address()+text.size();};
        for(auto at:{sites.visitTarget,sites.maskTarget,sites.stage,sites.refreshFlag,sites.refreshModels}) if(!inText(at)) {
            return fail({TransmogFailureCode::Code,false,{},std::format("Function address outside game text RVA={:#x}",at-base)});
        }
        std::uintptr_t firstVirtual{};
        if(!Read(sites.visitorTable,&firstVirtual,sizeof(firstVirtual)) || !Executable(firstVirtual)) {
            return fail({TransmogFailureCode::Code,false,{},"Worn visitor has unreadable/non-executable virtual function"});
        }
        const auto mask=Function(addresses.at(ids[1]));const auto root=Function(addresses.at(ids[0]));
        if(!mask || !root) return fail({TransmogFailureCode::Code,!mask,{},"Cannot decode bounded PE functions"});
        const auto offsets=MeasuredTransmogOffsets({version[0],version[1],version[2],version[3]});
        auto maskSite=Callsites::UniqueCall(*mask,0,[&](auto,auto target){return target==sites.maskTarget;});
        const auto maskMatches=std::count_if(mask->begin(),mask->end(),[&](const auto& ins){return ins.call==sites.maskTarget;});
        if(maskMatches>1) return fail({TransmogFailureCode::Callsite,true,{},"Multiple original hair calls"});
        if(!maskSite && offsets.mask) {
            if(const auto* call=Callsites::MeasuredCall(*mask,addresses.at(ids[1])+offsets.mask)) maskSite=call->at;
        }
        if(!maskSite) return fail(MissingCall(*mask,true));
        const auto maskChain=HookChain::Validate(*maskSite,sites.maskTarget,Read,Owner,Executable);
        if(!maskChain.direct) return fail({TransmogFailureCode::Target,true,{},std::format("Hair call target rejected RVA={:#x}",*maskSite-base)});
        // Discovery by target keeps the vanilla/IED identity rule. Other DLLs
        // can be chained at an independently verified, decoded measured site.
        auto knownWorn=[&](auto site,auto){return HookChain::Validate(site,sites.visitTarget,Read,Owner,Executable,false).direct!=0;};
        auto candidates=*root;
        // Older compilers keep a wrapper. Only inspect it if the real rebuild
        // parent CALLS it; never install in an unreferenced/dead wrapper.
        if(addresses.contains(ids[8]) && std::any_of(root->begin(),root->end(),[&](const auto& i){return i.call==addresses.at(ids[8]);})) {
            const auto wrapper=Function(addresses.at(ids[8]));
            if(!wrapper) return fail({TransmogFailureCode::Code,false,{},"Cannot decode referenced worn wrapper"});
            candidates.insert(candidates.end(),wrapper->begin(),wrapper->end());
        }
        // Require uniqueness across both live paths, including an ambiguous root.
        auto visitSite=Callsites::UniqueCall(candidates,sites.visitorTable,knownWorn);
        const auto wornMatches=std::count_if(candidates.begin(),candidates.end(),[&](const auto& ins){return ins.call && Callsites::HasAnchor(candidates,ins.at,sites.visitorTable) && knownWorn(ins.at,ins.call);});
        if(wornMatches>1) return fail({TransmogFailureCode::Callsite,false,{},"Multiple anchored original/IED worn calls"});
        if(!visitSite && offsets.worn) {
            const auto parent=se?(addresses.contains(ids[8])?addresses.at(ids[8]):0):addresses.at(ids[0]);
            if(parent) if(const auto* call=Callsites::MeasuredCall(candidates,parent+offsets.worn,sites.visitorTable)) visitSite=call->at;
        }
        if(!visitSite) return fail(MissingCall(candidates,false));
        const auto visitChain=HookChain::Validate(*visitSite,sites.visitTarget,Read,Owner,Executable);
        if(!visitChain.direct) return fail({TransmogFailureCode::Target,false,{},std::format("Worn call target rejected RVA={:#x}",*visitSite-base)});
        sites.maskSite=*maskSite;sites.visitSite=*visitSite;
        sites.maskCall=maskChain.direct;sites.visitCall=visitChain.direct;
        sites.maskModule=ModuleName(maskChain.resolved);sites.visitModule=ModuleName(visitChain.resolved);
        SKSE::log::info("RUNTIME4 previous callees preserved: hair={} worn={}; generic CALL chaining, no module-name blocklist",sites.maskModule,sites.visitModule);
        SKSE::log::info("RUNTIME4 validated runtime={} wornRVA={:#x} maskRVA={:#x} visitorRVA={:#x}; bounded reachable CALL sites verified by callee identity or measured layout",
            version.string(),sites.visitSite-base,sites.maskSite-base,sites.visitorTable-base);
        return sites;
    }catch(const std::exception& e) {return fail({TransmogFailureCode::Code,false,{},e.what()});}
}
}
