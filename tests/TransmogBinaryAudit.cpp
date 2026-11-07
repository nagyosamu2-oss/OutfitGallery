// Read-only offline evidence tool. Never loads/executes the game image.
// Address Library format definitions: CommonLibSSE-NG v10.1.0 REL/IDDB.{h,cpp}.
#include "TransmogCallsites.h"
#include "TransmogFunctionBounds.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
using namespace Gallery;
struct Reader {
    std::vector<std::uint8_t> b;std::size_t pos{};
    explicit Reader(const wchar_t* path) {
        std::ifstream f(std::filesystem::path(path),std::ios::binary);
        if(!f) throw std::runtime_error("file unavailable");
        b.assign(std::istreambuf_iterator<char>(f),{});
    }
    template<class T> T at(std::size_t p) const {
        if(p>b.size() || sizeof(T)>b.size()-p) throw std::runtime_error("truncated file");
        T v{};std::memcpy(&v,b.data()+p,sizeof v);return v;
    }
    template<class T> T get(){auto v=at<T>(pos);pos+=sizeof(T);return v;}
};
int wmain(int argc,wchar_t** argv) {try {
    if(argc!=3 && argc!=4) throw std::runtime_error("usage: TransmogBinaryAudit <exe or -> <address-library> [live-capture-directory]");
    Reader db(argv[2]);const auto format=db.get<std::uint32_t>();
    std::array<std::uint32_t,4> version{};for(auto& v:version)v=db.get<std::uint32_t>();
    const bool se=version[1]==5;
    std::map<std::uint64_t,std::uintptr_t> offsets;
    if(format==5) {
        db.pos+=64;if(db.get<std::uint32_t>()!=8) throw std::runtime_error("pointer size");
        db.get<std::uint32_t>();const auto count=db.get<std::uint32_t>();
        if(count>(db.b.size()-db.pos)/4) throw std::runtime_error("dense count");
        for(std::uint64_t i=0;i<count;++i){auto rva=db.get<std::uint32_t>();if(rva)offsets[i]=rva;}
    } else if(format==1 || format==2) {
        const auto len=db.get<std::uint32_t>();db.pos+=len;
        if(db.get<std::uint32_t>()!=8) throw std::runtime_error("pointer size");
        const auto count=db.get<std::uint32_t>();std::uint64_t id{},off{};
        if(count>db.b.size()) throw std::runtime_error("sparse count");
        auto value=[&](unsigned type,std::uint64_t prev)->std::uint64_t {
            switch(type) {
            case 0:return db.get<std::uint64_t>();case 1:return prev+1;
            case 2:return prev+db.get<std::uint8_t>();case 3:return prev-db.get<std::uint8_t>();
            case 4:return prev+db.get<std::uint16_t>();case 5:return prev-db.get<std::uint16_t>();
            case 6:return db.get<std::uint16_t>();case 7:return db.get<std::uint32_t>();
            default:throw std::runtime_error("encoding");}
        };
        for(unsigned i=0;i<count;++i) {const auto t=db.get<std::uint8_t>();id=value(t&15,id);
            off=value((t>>4)&7,(t&128)?off/8:off);if(t&128)off*=8;offsets[id]=off;}
    } else throw std::runtime_error("unknown format");
    const auto choose=[&](auto a,auto b){return se?a:b;};
    const std::array<unsigned,9> ids{choose(24221u,24725u),choose(24220u,24724u),choose(15856u,16096u),choose(15806u,16044u),
        choose(241890u,195851u),choose(17392u,17792u),choose(38867u,39907u),choose(38404u,39395u),choose(24231u,24735u)};
    std::cout<<"EXE target "<<version[0]<<'.'<<version[1]<<'.'<<version[2]<<'.'<<version[3]<<" format="<<format<<'\n';
    for(auto id:ids){if(!offsets.contains(id))throw std::runtime_error("missing ID");std::cout<<"  ID "<<id<<" RVA 0x"<<std::hex<<offsets.at(id)<<std::dec<<'\n';}
    if(std::wstring_view(argv[1])==L"-"){std::cout<<"PASS IDs only; game binary not checked\n";return 0;}
    Reader pe(argv[1]);auto nt=pe.at<std::uint32_t>(0x3c);
    if(pe.at<std::uint32_t>(nt)!=0x4550 || pe.at<std::uint16_t>(nt+24)!=0x20b) throw std::runtime_error("not PE64");
    auto sections=nt+24+pe.at<std::uint16_t>(nt+20);
    auto fileOffset=[&](std::uintptr_t rva)->std::size_t {
        for(unsigned i=0;i<pe.at<std::uint16_t>(nt+6);++i) {
            auto s=sections+40*i;auto start=pe.at<std::uint32_t>(s+12),size=pe.at<std::uint32_t>(s+16);
            if(rva>=start && rva-start<size)return pe.at<std::uint32_t>(s+20)+(rva-start);
        }throw std::runtime_error("unmapped RVA");
    };
    const auto pd=nt+24+112+3*8;
    const auto table=fileOffset(pe.at<std::uint32_t>(pd));const auto size=pe.at<std::uint32_t>(pd+4);
    auto lookup=[&](std::uint32_t rva)->std::optional<FunctionBounds::Entry> {
        for(std::size_t p=table;p+12<=table+size;p+=12) {
            const auto first=pe.at<std::uint32_t>(p),last=pe.at<std::uint32_t>(p+4);
            if(rva>=first && rva<last)return FunctionBounds::Entry{first,last,pe.at<std::uint32_t>(p+8)};
        }return {};
    };
    auto function=[&](std::uintptr_t rva) {
            const auto first=lookup(static_cast<std::uint32_t>(rva));
            if(!first || first->begin!=rva)throw std::runtime_error("no PE function entry");
            const auto bounds=FunctionBounds::Contiguous(*first,[&](std::uint32_t at,void* out,std::size_t n){
                auto f=fileOffset(at);if(f>pe.b.size() || n>pe.b.size()-f)return false;
                std::memcpy(out,pe.b.data()+f,n);return true;
            },lookup);
            if(!bounds)throw std::runtime_error("invalid chained unwind");
            const auto length=bounds->end-bounds->begin;
            std::cout<<"  function 0x"<<std::hex<<rva<<" firstEnd=0x"<<first->end<<" chainedEnd=0x"<<bounds->end<<std::dec<<'\n';
            auto f=fileOffset(rva);auto* data=&pe.b;
            std::optional<Reader> captured;
            if(argc==4) {
                std::wostringstream filename;filename<<L"function-"<<std::hex<<std::uppercase<<rva<<L".bin";
                captured.emplace((std::filesystem::path(argv[3])/filename.str()).c_str());data=&captured->b;f=0;
            }
            if(f+length>data->size())throw std::runtime_error("function file bounds");
            std::size_t failed{};
            const auto result=Callsites::Decode(std::span(*data).subspan(f,length),rva,&failed);
            if(!result) {
                std::cerr<<"decode failed at RVA 0x"<<std::hex<<(rva+failed)<<" bytes:";
                for(unsigned i=0;i<16 && f+failed+i<data->size();++i)std::cerr<<' '<<unsigned((*data)[f+failed+i]);
                std::cerr<<std::dec<<'\n';throw std::runtime_error("instruction decoding failed");
            }return *result;
    };
    const auto root=function(offsets.at(ids[0]));const auto mask=function(offsets.at(ids[1]));
    auto candidates=root;
    if(std::any_of(root.begin(),root.end(),[&](auto i){return i.call==offsets.at(ids[8]);})) {
        auto wrapper=function(offsets.at(ids[8]));candidates.insert(candidates.end(),wrapper.begin(),wrapper.end());
        std::cout<<"  live worn wrapper found\n";
    }
    auto visit=Callsites::UniqueCall(candidates,offsets.at(ids[4]),[&](auto,auto t){return t==offsets.at(ids[2]);});
    auto hair=Callsites::UniqueCall(mask,0,[&](auto,auto t){return t==offsets.at(ids[3]);});
    if(hair)std::cout<<"PASS hair mask identity: RVA=0x"<<std::hex<<*hair<<std::dec<<'\n';
    if(!visit)for(const auto& ins:candidates) {
        if(ins.call && std::any_of(candidates.begin(),candidates.end(),[&](const auto& prior){return prior.lea==offsets.at(ids[4]) && prior.at<ins.at && ins.at-prior.at<=0x60;}))
            std::cout<<"anchored call RVA=0x"<<std::hex<<ins.at<<" target-relative=0x"<<ins.call<<std::dec<<'\n';
    }
    if(!visit || !hair)throw std::runtime_error("missing or ambiguous callsites");
    std::cout<<"PASS bounded instruction audit: worn=0x"<<std::hex<<*visit<<" hair=0x"<<*hair<<std::dec<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
