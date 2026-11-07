#include "TransmogHookDiagnostics.h"
#include "TransmogHookChain.h"
#include "TransmogCallsites.h"
#include "TransmogRuntime.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void Require(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
std::vector<std::uint8_t> Load(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if(!input) throw std::runtime_error("Missing captured file");
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
int main(int argc,char** argv) {try {
    std::vector<std::uint8_t> memory(4096,0xCC);
    auto read=[&](std::uintptr_t at,void* out,std::size_t size) {
        if(at>memory.size() || size>memory.size()-at) return false;
        std::memcpy(out,memory.data()+at,size);return true;
    };
    auto emit=[&](std::size_t at,std::uint8_t opcode,std::uintptr_t to) {
        memory[at]=opcode;auto d=static_cast<std::int32_t>(to-at-5);std::memcpy(memory.data()+at+1,&d,4);
    };
    auto module=[](std::uintptr_t at)->std::string {return at==3000?"ExampleAppearance.dll":"";};
    // A real inline-jump shape must produce a reason, never be used as a CALL.
    emit(100,0xE9,500);emit(500,0xE9,700);
    memory[700]=0x51; // generated block preserves rcx, then indirect-calls its DLL
    memory[701]=0xFF;memory[702]=0x15;
    const std::int32_t displacement=93;std::memcpy(memory.data()+703,&displacement,4);
    std::uintptr_t target=3000;std::memcpy(memory.data()+800,&target,8);
    Require(HookDiagnostics::RelatedModule(500,read,module)=="ExampleAppearance.dll","decoded generated block identifies related DLL");
    Require(!HookChain::Validate(100,2500,read,[](auto){return HookChain::Owner::Other;},[](auto){return true;}).direct,"inline jump cannot masquerade as a function call");
    emit(500,0xE9,500);Require(HookDiagnostics::RelatedModule(500,read,module).empty(),"diagnostic jump cycle terminates");
    memory[700]=0xC3;Require(HookDiagnostics::RelatedModule(700,read,module).empty(),"no attribution from data after return");
    Require(HookDiagnostics::RelatedModule(4090,read,module).empty(),"unreadable diagnostic body terminates");

    // Measured fallback cannot select immediate bytes, an unreachable call,
    // an inline jump, or the wrong visitor, even if the hint lands there.
    std::vector<std::uint8_t> code(48,0x90);
    code[0]=0x48;code[1]=0xB8;code[2]=0xE8;
    code[10]=0x48;code[11]=0x8D;code[12]=0x05;
    const std::int32_t lea=0x3000-(0x1000+17);std::memcpy(code.data()+13,&lea,4);
    code[20]=0xE8;const std::int32_t call=0x4000-(0x1000+25);std::memcpy(code.data()+21,&call,4);code[25]=0xC3;code[30]=0xE8;
    auto decoded=Callsites::Decode(code,0x1000);Require(bool(decoded),"fixture decoded");
    Require(Callsites::MeasuredCall(*decoded,0x1014,0x3000)!=nullptr,"foreign callee at measured anchored call accepted");
    Require(!Callsites::MeasuredCall(*decoded,0x1002),"immediate E8 excluded");
    Require(!Callsites::MeasuredCall(*decoded,0x101E),"unreachable E8 excluded");
    Require(!Callsites::MeasuredCall(*decoded,0x1014,0x3001),"different visitor excluded");
    code[20]=0xE9;decoded=Callsites::Decode(code,0x1000);
    Require(decoded && !Callsites::MeasuredCall(*decoded,0x1014),"inline jump excluded from fallback");
    Require(MeasuredTransmogOffsets({1,5,97,0}).mask==0x7C,"SE measured layout");
    Require(MeasuredTransmogOffsets({1,6,1170,0}).worn==0x1EF,"AE measured layout");
    const auto steam104 = MeasuredTransmogOffsets({1,7,104,0});
    Require(steam104.mask==0x80 && steam104.worn==0x1EF,"Steam 1.7.104 audited layout");
    Require(!MeasuredTransmogOffsets({1,6,640,0}).mask,"unmeasured runtimes not assigned guessed foreign sites");
    const TransmogFailure failure{TransmogFailureCode::Jump,true,"ExampleAppearance.dll",{}};
    Require(DescribeTransmogFailure(failure,true).find("ExampleAppearance.dll")!=std::string::npos,"localized reason retains module identity");
    Require(DescribeTransmogFailure(failure,false).find("hook format")!=std::string::npos,"reason describes hook format instead of blaming runtime");

    if(argc==2) {
        const std::filesystem::path folder(argv[1]);
        std::ifstream input(folder/"modules.json");nlohmann::json modules;input>>modules;
        std::uintptr_t image{};
        for(const auto& m:modules) if(m.at("Name")=="SkyrimSE.exe") image=m.at("Base").get<std::uintptr_t>();
        Require(image!=0,"captured game base present");
        auto name=[&](std::uintptr_t at)->std::string {
            for(const auto& m:modules) {
                auto base=m.at("Base").get<std::uintptr_t>(),size=m.at("Size").get<std::uintptr_t>();
                if(at>=base && at-base<size && m.at("Name")!="SkyrimSE.exe") return m.at("Name").get<std::string>();
            }
            return {};
        };
        for(const auto& stem:{std::string("362F0C"),std::string("364301")}) {
            const auto rva=std::stoull(stem,nullptr,16);
            const auto parent=rva==0x362F0C?0x362E90:0x364280;
            auto function=Load(folder/(parent==0x362E90?"function-362E90.bin":"function-364280.bin"));
            // Captured PE/unwind ends: use the same bounded reachable decoding
            // as startup, rather than attributing a manually supplied target.
            const auto end=parent==0x362E90?0x362FFA:0x364347;
            const auto instructions=Callsites::Decode(std::span(function).first(end-parent),image+parent);
            Require(bool(instructions),"captured bounded function decoded");
            const auto external=std::find_if(instructions->begin(),instructions->end(),[&](const auto& ins){
                return ins.jump && std::none_of(instructions->begin(),instructions->end(),[&](const auto& next){return next.at==ins.jump;});
            });
            Require(external!=instructions->end() && external->at==image+rva,"startup diagnostic selects observed conflicting jump");
            const auto offset=rva-parent;Require(function[offset]==0xE9,"observed inline jump");
            std::int32_t delta{};std::memcpy(&delta,function.data()+offset+1,sizeof(delta));
            const auto direct=image+rva+5+delta;
            auto first=Load(folder/("site-"+stem+"-hop-0.bin"));
            Require(first[0]==0xFF && first[1]==0x25,"captured plain pointer jump");
            std::uintptr_t generated{};std::memcpy(&generated,first.data()+6,sizeof(generated));
            auto second=Load(folder/("site-"+stem+"-hop-1.bin"));
            auto captureRead=[&](std::uintptr_t at,void* out,std::size_t size) {
                for(auto pair:{std::pair{direct,&first},std::pair{generated,&second}}) {
                    if(at>=pair.first && at-pair.first<=pair.second->size() && size<=pair.second->size()-(at-pair.first)) {
                        std::memcpy(out,pair.second->data()+(at-pair.first),size);return true;
                    }
                }
                return false;
            };
            auto related=HookDiagnostics::RelatedModule(direct,captureRead,name);
            Require(related=="SkyrimOutfitSystemSE.dll","captured hook attributed to Skyrim Outfit System");
            std::cout<<"Captured "<<stem<<": "<<DescribeTransmogFailure({TransmogFailureCode::Jump,rva==0x362F0C,related,{}},false)<<'\n';
        }
    }
    std::cout<<"Compatibility selection and failure diagnostics passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
