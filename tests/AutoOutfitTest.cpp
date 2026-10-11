#include "AutoOutfitPolicy.h"
#include "Presets.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
using namespace Gallery;
using namespace Gallery::AutoOutfit;
void require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
int main() {
 try {
    const AutoOutfitSlots slots{"adventure.png","town.png","home.png","inn.png","combat.png","night.png","rain.png","snow.png","dungeon.png","temple.png"};
    require(Choose(Place::Home,slots,true)=="home.png","home priority");
    require(Choose(Place::Town,slots,true)=="town.png","town priority");
    require(Choose(Place::Normal,slots,false)=="adventure.png","default priority");
    require(Choose(Place::Home,{"default.png","town.png",""},true)=="town.png","home fallback");
    require(Choose(Place::Home,{"default.png","town.png",""},false)=="town.png","home fallback outside town");
    require(Choose(Place::Town,{"default.png","",""},true)=="default.png","town fallback");
    require(Choose(Place::Normal,{},false).empty(),"unassigned");
    require(Classify(false,false,false)==Place::Normal,"classify default");
    require(Classify(false,false,true)==Place::Town,"classify town");
    require(Classify(false,true,true)==Place::Inn,"inn outranks parent town");
    require(Classify(false,true,false)==Place::Inn,"standalone inn");
    require(Classify(true,true,true)==Place::Home,"home wins ambiguous keywords");
    require(Choose(Place::Inn,slots,true)=="inn.png","inn assigned in town");
    require(Choose(Place::Inn,slots,false)=="inn.png","inn assigned outside town");
    require(Choose(Place::Inn,{"default.png","town.png","home.png",""},false)=="town.png","inn empty fallback never home");
    require(Choose(Place::Inn,{"default.png","","home.png",""},true)=="default.png","inn default fallback");
    require(Choose(Place::Home,{"default.png","town.png","","inn.png"},true)=="town.png","home empty never uses inn sibling");
    Policy p;
    require(p.Observe(1,1000,0) && p.Attempt("town.png",1000),"town entry");p.Applied("town.png");
    require(p.Observe(3,6000,0) && p.Attempt("inn.png",6000),"inn entry");p.Applied("inn.png");
    require(!p.Observe(3,12000,0),"inn no repeat");
    p.Manual(3);require(!p.Observe(3,16000,0),"manual inn outfit protected");
    require(p.Observe(1,17000,0) && p.Attempt("town.png",17000),"leaving inn restores town rule");
    p={};
    require(!p.Observe(0,1000) && !p.Observe(0,3999) && p.Observe(0,4000),"stable delay");
    require(p.Attempt("a.png",4000),"initial attempt"); p.Applied("a.png");
    require(!p.Observe(0,100000),"no repeated apply");
    require(!p.Observe(1,101000) && !p.Observe(0,102000),"boundary jitter");
    require(!p.Observe(1,103000) && p.Observe(1,106000),"stable new category");
    require(!p.Attempt("a.png",106000),"same set across categories");
    p.Manual(1); require(!p.Observe(1,107000) && !p.Observe(1,111000),"manual priority");
    p.Block(); require(!p.Observe(1,120000) && !p.Observe(1,124000),"manual survives pause");
    require(!p.Observe(2,125000) && p.Observe(2,128000),"manual releases on change");
    require(p.Attempt("a.png",128000),"manual invalidates previous set cache");
    // Simulate failure: attempted but never Applied. Do not loop on unchanged category.
    require(!p.Observe(2,200000),"failure no retry");
    p.Block(); require(!p.Observe(2,201000) && !p.Observe(2,205000),"failure survives blocked state");
    p={}; require(!p.Observe(0,1) && p.Observe(0,3001),"reset allows fresh start");
    require(p.Attempt("x.png",3001),"fresh attempt");
    require(!p.Observe(1,3100) && !p.Observe(1,6100) && p.Observe(1,8001),"cooldown");
    p={}; require(!p.Observe(0,1),"first sample"); p.Block();
    require(!p.Observe(0,5000) && !p.Observe(0,7999) && p.Observe(0,8000),"blocked requires fresh stability");
    p={}; require(p.Observe(0,1000,0),"zero delay fires on first poll");
    require(p.Attempt("fast.png",1000),"zero delay attempt"); p.Applied("fast.png");
    require(!p.Observe(0,2000,0),"zero does not repeat");
    require(!p.Observe(1,2000,0) && p.Observe(1,6000,0),"zero retains cooldown");
    p.Manual(1); require(!p.Observe(1,10000,0),"zero retains manual priority");
    p={}; require(!p.Observe(0,1000,10000) && !p.Observe(0,10999,10000) && p.Observe(0,11000,10000),"ten second delay");
    p={}; require(!p.Observe(0,1000,10000) && p.Observe(0,2000,1000),"shorten pending wait");
    require(p.Attempt("changed.png",2000),"shortened delay attempt");
    require(!p.Observe(0,30000,10000),"changing delay never rearms handled category");
    p={}; require(p.Observe(0,1000,0),"zero before block"); p.Block();
    require(!p.Observe(0,2000,1000) && p.Observe(0,3000,1000),"blocked delay measured again");
    const auto folder=std::filesystem::temp_directory_path()/"OutfitGallery-AutoPolicyTest";
    std::filesystem::create_directories(folder); auto file=folder/"AutoOutfits.json";
    for(bool hide:{false,true}) {
        AutoOutfitSettings prefs{slots,2,4,15,true,true,true,hide};
        WriteAutoOutfitSettings(prefs,file); const auto read=ReadAutoOutfitSettings(file);
        require(read.hideAssignButtons==hide && read.slots==slots && read.enabled && read.paused && read.combatEnabled && read.ostimRecoverySeconds==15,"display preference preserves automatic outfit settings");
    }
    WriteAutoOutfitSettings({slots,3,3},file); require(ReadAutoOutfitSettings(file).slots==slots,"persistence");
    for(const auto& bad:{"../outside.png","C:/outside.png","sub\\file.png","not-json.txt"}) {
        bool rejected=false; try {WriteAutoOutfitSettings({{bad,"",""},3,3},file);} catch(const std::exception&) {rejected=true;}
        require(rejected,"unsafe photo rejected"); require(ReadAutoOutfitSettings(file).slots==slots,"invalid write leaves existing file");
    }
    {std::ofstream out(file); out<<R"({"schema":1,"slots":["a.png"]})";}
    bool rejected=false; try {(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){rejected=true;}
    require(rejected,"wrong slot count");
    {std::ofstream out(file); out<<R"({"schema":1,"slots":["a.png","b.png",""]})";}
    const auto legacy=ReadAutoOutfitSettings(file);
    require(!legacy.hideAssignButtons,"old settings keep assignment buttons visible");
    require(legacy.delaySeconds==3 && legacy.columns==3 && legacy.slots[0]=="a.png" && legacy.slots[1]=="b.png" && legacy.slots[3].empty(),"legacy defaults retain assignments");
    {std::ofstream out(file); out<<R"({"schema":1,"slots":["a.png","b.png","c.png"],"delaySeconds":0,"columns":5})";}
    const auto previous=ReadAutoOutfitSettings(file);
    require(previous.slots[0]=="a.png" && previous.slots[1]=="b.png" && previous.slots[2]=="c.png" && previous.slots[3].empty() && previous.delaySeconds==0 && previous.columns==5,"auto-test2 migration retains all settings");
    {std::ofstream out(file); out<<R"({"schema":1,"slots":["a.png","b.png","c.png"],"innPhoto":"../bad.png"})";}
    rejected=false;try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){rejected=true;}
    require(rejected,"unsafe inn photo rejected");
    {std::ofstream out(file); out<<R"({"schema":1,"slots":["a.png","b.png","c.png"],"innPhoto":"inn.png","delaySeconds":0,"columns":5})";}
    const auto test3=ReadAutoOutfitSettings(file);
    require(test3.slots[3]=="inn.png" && test3.slots[4].empty() && test3.delaySeconds==0 && test3.columns==5,"test3 migration preserves inn and preferences, combat unassigned");
    for(const auto bad:{R"({"schema":1,"slots":["","",""],"combatPhoto":"../bad.png"})",R"({"schema":1,"slots":["","",""],"combatPhoto":true})"}) {
        {std::ofstream out(file); out<<bad;}
        rejected=false;try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){rejected=true;}
        require(rejected,"unsafe or wrong-type combat photo rejected");
    }
    WriteAutoOutfitSettings({slots,0,5},file);
    auto unsafeCombat=slots;unsafeCombat[4]="C:/bad.png";
    rejected=false;try{WriteAutoOutfitSettings({unsafeCombat,0,5},file);}catch(const std::exception&){rejected=true;}
    require(rejected && ReadAutoOutfitSettings(file).slots==slots,"invalid combat write preserves settings");
    {std::ofstream out(file);out<<R"({"schema":1,"slots":["a.png","b.png","c.png"],"innPhoto":"inn.png","combatPhoto":"combat.png","delaySeconds":0,"columns":4})";}
    const auto test5=ReadAutoOutfitSettings(file);
    require(test5.slots[3]=="inn.png" && test5.slots[4]=="combat.png" && test5.slots[5].empty() && test5.slots[6].empty() && test5.slots[7].empty() && test5.delaySeconds==0 && test5.columns==4,"test5 migration retains existing fields with new slots empty");
    {std::ofstream out(file);out<<R"({"schema":1,"slots":["a.png","b.png","c.png"],"innPhoto":"inn.png","combatPhoto":"combat.png","nightPhoto":"night.png","rainPhoto":"rain.png","snowPhoto":"snow.png","delaySeconds":0,"columns":5})";}
    const auto test6=ReadAutoOutfitSettings(file);
    require(test6.slots[7]=="snow.png" && test6.slots[8].empty() && test6.slots[9].empty() && test6.columns==5 && test6.delaySeconds==0,"test6 migration leaves new places empty");
    for(const auto key:{"nightPhoto","rainPhoto","snowPhoto","dungeonPhoto","templePhoto"}) {
        for(const auto value:{"\"../outside.png\"","true","null"}) {
            {std::ofstream out(file);out<<"{\"schema\":1,\"slots\":[\"\",\"\",\"\"],\""<<key<<"\":"<<value<<"}";}
            rejected=false;try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){rejected=true;}
            require(rejected,"invalid climate assignment rejected");
        }
    }
    for(int delay:{0,1,3,10}) for(int columns:{3,4,5}) {
        WriteAutoOutfitSettings({slots,delay,columns},file); const auto settings=ReadAutoOutfitSettings(file);
        require(settings.slots==slots && settings.delaySeconds==delay && settings.columns==columns,"preferences roundtrip");
    }
    for(const auto invalid: {AutoOutfitSettings{slots,-1,3},AutoOutfitSettings{slots,11,3},AutoOutfitSettings{slots,3,2},AutoOutfitSettings{slots,3,6}}) {
        rejected=false; try {WriteAutoOutfitSettings(invalid,file);}catch(const std::exception&){rejected=true;}
        require(rejected,"invalid preferences rejected"); require(ReadAutoOutfitSettings(file).delaySeconds==10,"failed write preserves existing settings");
    }
    for(const auto bad:{R"({"schema":1,"slots":["","",""],"delaySeconds":-1})",R"({"schema":1,"slots":["","",""],"delaySeconds":11})",R"({"schema":1,"slots":["","",""],"columns":6})",R"({"schema":1,"slots":["","",""],"delaySeconds":0.5})",R"({"schema":1,"slots":["","",""],"columns":true})"}) {
        {std::ofstream out(file); out<<bad;}
        rejected=false; try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){rejected=true;}
        require(rejected,"invalid persisted settings rejected");
    }
    {std::ofstream out(file);out<<R"({"schema":1,"slots":["","",""]})";}
    require(ReadAutoOutfitSettings(file).ostimRecoverySeconds==10,"old settings default to ten second recovery");
    for(int recovery:{5,10,30}) {
        WriteAutoOutfitSettings({slots,0,4,recovery},file);
        const auto read=ReadAutoOutfitSettings(file);
        require(read.ostimRecoverySeconds==recovery && read.slots==slots && read.delaySeconds==0 && read.columns==4,"recovery persistence preserves assignments and preferences");
    }
    for(int invalid:{-1,0,4,31}) {
        bool invalidRejected=false;
        try{WriteAutoOutfitSettings({slots,0,4,invalid},file);}catch(const std::exception&){invalidRejected=true;}
        require(invalidRejected && ReadAutoOutfitSettings(file).ostimRecoverySeconds==30,"invalid recovery preserves settings");
    }
    for(const auto bad:{"4","31","5.5","true","4294967306"}) {
        {std::ofstream out(file);out<<"{\"schema\":1,\"slots\":[\"\",\"\",\"\"],\"ostimRecoverySeconds\":"<<bad<<"}";}
        bool invalidRejected=false;try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){invalidRejected=true;}
        require(invalidRejected,"bad persisted recovery rejected");
    }
    AutoOutfitSlots extended=slots;
    for(unsigned i=10;i<extended.size();++i)extended[i]="extra"+std::to_string(i)+".png";
    WriteAutoOutfitSettings({extended,0,5,20},file);
    auto expanded=ReadAutoOutfitSettings(file);
    require(expanded.slots==extended && expanded.ostimRecoverySeconds==20 && expanded.columns==5,"all twenty-seven assignments persist");
    {std::ofstream out(file);out<<R"({"schema":1,"slots":["old.png","",""],"dungeonPhoto":"dungeon.png","ostimRecoverySeconds":15})";}
    auto migrated=ReadAutoOutfitSettings(file);
    require(migrated.slots[0]=="old.png" && migrated.slots[8]=="dungeon.png" && migrated.ostimRecoverySeconds==15,"old trial settings retained");
    for(unsigned i=10;i<migrated.slots.size();++i)require(migrated.slots[i].empty(),"new conditions default unassigned");
    for(const auto key:{"swimmingPhoto","dawnguardPhoto","collegePhoto","brotherhoodPhoto","companionsPhoto","bardsPhoto","thievesPhoto","bluePalacePhoto","meleePhoto","magicPhoto","archeryPhoto","unarmedPhoto","sneakingPhoto","smithingPhoto","alchemyPhoto","enchantingPhoto","shopPhoto"}) {
        {std::ofstream out(file);out<<"{\"schema\":1,\"slots\":[\"\",\"\",\"\"],\""<<key<<"\":\"../bad.png\"}";}
        bool invalidRejected=false;try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){invalidRejected=true;}
        require(invalidRejected,"new assignments validate photo paths");
    }
    {std::ofstream out(file);out<<R"({"schema":1,"slots":["old.png","",""],"swimmingPhoto":"swim.png","thievesPhoto":"guild.png"})";}
    {
    const auto previous=ReadAutoOutfitSettings(file);
    require(previous.slots[10]=="swim.png" && previous.slots[16]=="guild.png" && previous.slots[17].empty(),"test11 migration preserves swimming and guild, palace unassigned");
    }
    require(!ReadAutoOutfitSettings(file).enabled && !ReadAutoOutfitSettings(file).paused && !ReadAutoOutfitSettings(file).combatEnabled,"legacy preferences stay opt-in");
    for(bool enabled:{false,true}) for(bool paused:{false,true}) for(bool combat:{false,true}) {
        WriteAutoOutfitSettings({extended,2,4,12,enabled,paused,combat},file);
        const auto saved=ReadAutoOutfitSettings(file);
        require(saved.enabled==enabled && saved.paused==paused && saved.combatEnabled==combat && saved.slots==extended,"all toggle combinations and assignments persist");
    }
    for(const auto* key:{"enabled","paused","combatEnabled","hideAssignButtons"}) for(const auto* value:{"1","null","\"true\""}) {
        {std::ofstream out(file);out<<"{\"schema\":1,\"slots\":[\"\",\"\",\"\"],\""<<key<<"\":"<<value<<"}";}
        bool invalid=false;try{(void)ReadAutoOutfitSettings(file);}catch(const std::exception&){invalid=true;}
        require(invalid,"toggle values must be boolean");
    }
    {std::ofstream out(file);out<<R"({"schema":1,"slots":["old.png","",""],"delaySeconds":10})";}
    require(ReadAutoOutfitSettings(file).sneakDelaySeconds==2,"legacy settings default to two seconds independently of normal delay");
    for(int delay:{0,2,5}) {
        AutoOutfitSettings settings{extended,10,5,20,true,true,true,true};settings.sneakDelaySeconds=delay;
        WriteAutoOutfitSettings(settings,file);const auto read=ReadAutoOutfitSettings(file);
        require(read.sneakDelaySeconds==delay && read.delaySeconds==10 && read.slots==extended && read.enabled && read.paused && read.combatEnabled && read.hideAssignButtons,"sneak delay roundtrip preserves other preferences");
    }
    for(int invalid:{-1,6}) {
        auto settings=ReadAutoOutfitSettings(file);settings.sneakDelaySeconds=invalid;
        bool rejected=false;try{WriteAutoOutfitSettings(settings,file);}catch(const std::exception&){rejected=true;}
        require(rejected && ReadAutoOutfitSettings(file).sneakDelaySeconds==5,"invalid write preserves previous settings");
    }
    for(const auto* value:{"-1","6","1.5","true","null","4294967296"}) {
        {std::ofstream out(file);out<<"{\"schema\":1,\"slots\":[\"\",\"\",\"\"],\"sneakDelaySeconds\":"<<value<<"}";}
        bool rejected=false;try{ReadAutoOutfitSettings(file);}catch(const std::exception&){rejected=true;}
        require(rejected,"invalid persisted sneak delay rejected");
    }

    {std::ofstream out(file);out<<R"({"schema":1,"slots":["old.png","",""]})";}
    require(ReadAutoOutfitSettings(file).combatIntervalSeconds==5,"legacy combat interval defaults to five");
    for(int interval:{2,5,10}) {
        AutoOutfitSettings settings{extended,3,4,12,true,false,true,true};
        settings.combatIntervalSeconds=interval;
        WriteAutoOutfitSettings(settings,file);
        const auto read=ReadAutoOutfitSettings(file);
        require(read.combatIntervalSeconds==interval && read.slots==extended && read.delaySeconds==3 && read.combatEnabled,"combat interval roundtrip preserves preferences");
    }
    for(int interval:{1,11}) {
        auto settings=ReadAutoOutfitSettings(file);settings.combatIntervalSeconds=interval;
        bool invalid=false;try{WriteAutoOutfitSettings(settings,file);}catch(const std::exception&){invalid=true;}
        require(invalid && ReadAutoOutfitSettings(file).combatIntervalSeconds==10,"invalid interval write preserves settings");
    }
    for(const auto* value:{"1","11","2.5","true","null","4294967298"}) {
        {std::ofstream out(file);out<<"{\"schema\":1,\"slots\":[\"\",\"\",\"\"],\"combatIntervalSeconds\":"<<value<<"}";}
        bool invalid=false;try{ReadAutoOutfitSettings(file);}catch(const std::exception&){invalid=true;}
        require(invalid,"invalid persisted combat interval rejected");
    }
    std::filesystem::remove(file); std::filesystem::remove(folder);
    require(Classify(false,false,false,true,false,false)==Place::Normal,"outdoor dungeon approach stays normal");
    require(Classify(false,false,true,true,false,false)==Place::Town,"outdoor dungeon retains town");
    require(Classify(false,false,false,true,false,true)==Place::Dungeon,"interior dungeon applies");
    std::cout<<"PASS: priority, debounce, transitions, manual override, no retries, cooldown, reset, assignment persistence\n";
    return 0;
 }catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}
}


