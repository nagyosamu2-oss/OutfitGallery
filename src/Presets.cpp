#include "Presets.h"
#include "PresetEditing.h"
#include <chrono>
#include <atomic>
#include "DeletionPath.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <set>
#include <cmath>
#include <algorithm>
#include <cctype>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
namespace Gallery {
using Json = nlohmann::json;
static void ValidateAutoSlots(const AutoOutfitSlots& slots) {
    for(const auto& photo:slots) {
        if(photo.empty()) continue;
        if(photo.size()>240 || photo.find_first_of("/\\:")!=std::string::npos || photo.find('\0')!=std::string::npos ||
           photo=="." || photo==".." || std::filesystem::path(photo).extension()!=".png")
            throw std::runtime_error("Invalid auto outfit photo");
    }
}
AutoOutfitSettings ReadAutoOutfitSettings(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>8192) throw std::runtime_error("Auto outfit settings too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1 || !j.at("slots").is_array() || j.at("slots").size()!=3)
        throw std::runtime_error("Invalid auto outfit settings");
    AutoOutfitSettings settings;
    const auto legacySlots=j.at("slots").get<std::array<std::string,3>>();
    std::copy(legacySlots.begin(),legacySlots.end(),settings.slots.begin());
    settings.slots[3]=j.value("innPhoto",std::string{});
    settings.slots[4]=j.value("combatPhoto",std::string{});
    settings.slots[5]=j.value("nightPhoto",std::string{});
    settings.slots[6]=j.value("rainPhoto",std::string{});
    settings.slots[7]=j.value("snowPhoto",std::string{});
    settings.slots[8]=j.value("dungeonPhoto",std::string{});
    settings.slots[9]=j.value("templePhoto",std::string{});
    settings.slots[10]=j.value("swimmingPhoto",std::string{});
    settings.slots[11]=j.value("dawnguardPhoto",std::string{});
    settings.slots[12]=j.value("collegePhoto",std::string{});
    settings.slots[13]=j.value("brotherhoodPhoto",std::string{});
    settings.slots[14]=j.value("companionsPhoto",std::string{});
    settings.slots[15]=j.value("bardsPhoto",std::string{});
    settings.slots[16]=j.value("thievesPhoto",std::string{});
    settings.slots[17]=j.value("bluePalacePhoto",std::string{});
    settings.slots[18]=j.value("meleePhoto",std::string{});
    settings.slots[19]=j.value("magicPhoto",std::string{});
    settings.slots[20]=j.value("archeryPhoto",std::string{});
    settings.slots[21]=j.value("unarmedPhoto",std::string{});
    settings.slots[22]=j.value("sneakingPhoto",std::string{});
    settings.slots[23]=j.value("smithingPhoto",std::string{});
    settings.slots[24]=j.value("alchemyPhoto",std::string{});
    settings.slots[25]=j.value("enchantingPhoto",std::string{});
    settings.slots[26]=j.value("shopPhoto",std::string{});
    for(const auto* key:{"enabled","paused","combatEnabled","hideAssignButtons"}) if(j.contains(key) && !j.at(key).is_boolean())
        throw std::runtime_error("Invalid auto outfit toggle setting");
    settings.hideAssignButtons=j.value("hideAssignButtons",false); settings.enabled=j.value("enabled",false); settings.paused=j.value("paused",false); settings.combatEnabled=j.value("combatEnabled",false);
    ValidateAutoSlots(settings.slots);
    for(const auto* key:{"delaySeconds","columns","ostimRecoverySeconds","sneakDelaySeconds","combatIntervalSeconds"}) if(j.contains(key) && !j.at(key).is_number_integer())
        throw std::runtime_error("Invalid auto outfit numeric setting");
    if((j.contains("ostimRecoverySeconds") && (j["ostimRecoverySeconds"]<5 || j["ostimRecoverySeconds"]>30)) ||
       (j.contains("delaySeconds") && (j["delaySeconds"]<0 || j["delaySeconds"]>10)) ||
       (j.contains("columns") && (j["columns"]<3 || j["columns"]>5)))
        throw std::runtime_error("Auto outfit setting out of range");
    if(j.contains("sneakDelaySeconds") && (j["sneakDelaySeconds"]<0 || j["sneakDelaySeconds"]>5)) throw std::runtime_error("Auto outfit setting out of range");
    if(j.contains("combatIntervalSeconds") && (j["combatIntervalSeconds"]<2 || j["combatIntervalSeconds"]>10)) throw std::runtime_error("Auto outfit setting out of range");
    settings.combatIntervalSeconds=j.value("combatIntervalSeconds",5);
    settings.sneakDelaySeconds=j.value("sneakDelaySeconds",2);
    settings.ostimRecoverySeconds=j.value("ostimRecoverySeconds",10);
    settings.delaySeconds=j.value("delaySeconds",3); settings.columns=j.value("columns",3);
    if(settings.combatIntervalSeconds<2 || settings.combatIntervalSeconds>10 || settings.sneakDelaySeconds<0 || settings.sneakDelaySeconds>5 || settings.ostimRecoverySeconds<5 || settings.ostimRecoverySeconds>30 || settings.delaySeconds<0 || settings.delaySeconds>10 || settings.columns<3 || settings.columns>5)
        throw std::runtime_error("Auto outfit setting out of range");
    return settings;
}
void WriteAutoOutfitSettings(const AutoOutfitSettings& settings,const std::filesystem::path& path) {
    ValidateAutoSlots(settings.slots);
    if(settings.combatIntervalSeconds<2 || settings.combatIntervalSeconds>10 || settings.sneakDelaySeconds<0 || settings.sneakDelaySeconds>5 || settings.ostimRecoverySeconds<5 || settings.ostimRecoverySeconds>30 || settings.delaySeconds<0 || settings.delaySeconds>10 || settings.columns<3 || settings.columns>5)
        throw std::runtime_error("Auto outfit setting out of range");
    // Keep schema-1's original three slots so earlier trial versions can still read them.
    const std::array<std::string,3> legacySlots{settings.slots[0],settings.slots[1],settings.slots[2]};
    Json j={{"schema",1},{"slots",legacySlots},{"innPhoto",settings.slots[3]},{"combatPhoto",settings.slots[4]},{"nightPhoto",settings.slots[5]},{"rainPhoto",settings.slots[6]},{"snowPhoto",settings.slots[7]},{"dungeonPhoto",settings.slots[8]},{"templePhoto",settings.slots[9]},{"swimmingPhoto",settings.slots[10]},{"dawnguardPhoto",settings.slots[11]},{"collegePhoto",settings.slots[12]},{"brotherhoodPhoto",settings.slots[13]},{"companionsPhoto",settings.slots[14]},{"bardsPhoto",settings.slots[15]},{"thievesPhoto",settings.slots[16]},{"bluePalacePhoto",settings.slots[17]},{"meleePhoto",settings.slots[18]},{"magicPhoto",settings.slots[19]},{"archeryPhoto",settings.slots[20]},{"unarmedPhoto",settings.slots[21]},{"sneakingPhoto",settings.slots[22]},{"smithingPhoto",settings.slots[23]},{"alchemyPhoto",settings.slots[24]},{"enchantingPhoto",settings.slots[25]},{"shopPhoto",settings.slots[26]},{"combatIntervalSeconds",settings.combatIntervalSeconds},{"delaySeconds",settings.delaySeconds},{"sneakDelaySeconds",settings.sneakDelaySeconds},{"columns",settings.columns},{"ostimRecoverySeconds",settings.ostimRecoverySeconds},{"enabled",settings.enabled},{"paused",settings.paused},{"combatEnabled",settings.combatEnabled},{"hideAssignButtons",settings.hideAssignButtons}};
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp,std::ios::binary); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Could not save auto outfits");
}
bool SameEquipment(const Preset& a,const Preset& b) {
    auto keys=[](const Preset& p) {
        std::multiset<std::string> result;
        for(const auto& i:p.items) {auto plugin=i.plugin; std::transform(plugin.begin(),plugin.end(),plugin.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));}); result.insert(plugin+":"+std::to_string(i.localID)+":"+i.kind+":"+i.hand);}
        return result;
    };
    return keys(a)==keys(b);
}
void CheckInput(const InputSettings& s) {
    if(s.autoModifiers>7 || (s.autoKeyboard && (s.autoKeyboard<2 || s.autoKeyboard>255))) throw std::runtime_error("Invalid auto panel key");
    if(s.autoKeyboard && s.autoKeyboard==s.keyboard && s.autoModifiers==s.keyboardModifiers) throw std::runtime_error("Auto panel key conflicts with Gallery");
    if(s.autoGamepad && s.autoGamepad==s.gamepad) throw std::runtime_error("Auto panel button conflicts with Gallery");
    if(s.keyboardModifiers>7) throw std::runtime_error("Invalid keyboard modifiers");
    std::set<unsigned> usedKeys{s.keyboard},usedPads{1,2,4,8,9,10,128,256,512,4096,8192};
    if(s.gamepad) usedPads.insert(s.gamepad);
    if(s.autoKeyboard) usedKeys.insert(s.autoKeyboard);
    if(s.autoGamepad) usedPads.insert(s.autoGamepad);
    for(auto key:s.captureKeys) if(key && (key<2 || key>255 || !usedKeys.insert(key).second)) throw std::runtime_error("Capture key conflicts with another action.");
    for(auto key:s.capturePads) if(key && ((key!=16 && key!=32 && key!=64 && key!=16384 && key!=32768) || !usedPads.insert(key).second)) throw std::runtime_error("Capture button is reserved or already assigned.");
    const std::set<unsigned> buttons{0,1,2,4,8,9,10,16,32,64,128,256,512,4096,8192,16384,32768};
    if(!buttons.contains(s.autoGamepad) || !std::isfinite(s.autoHoldSeconds) || s.autoHoldSeconds<.4f || s.autoHoldSeconds>3.f) throw std::runtime_error("Invalid auto panel hold/button");
    if(s.keyboard<2 || s.keyboard>255 || !buttons.contains(s.gamepad) || !std::isfinite(s.holdSeconds) || s.holdSeconds<.4f || s.holdSeconds>3.f) throw std::runtime_error("Invalid hotkey settings");
}
InputSettings ReadInputSettings(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>4096) throw std::runtime_error("Hotkey file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1) throw std::runtime_error("Unsupported hotkey schema");
    if(j.contains("keyboardModifiers") && (!j.at("keyboardModifiers").is_number_integer() || j["keyboardModifiers"]<0 || j["keyboardModifiers"]>7))
        throw std::runtime_error("Invalid keyboard modifiers");
    InputSettings s{j.at("keyboard"),j.at("gamepad"),j.at("holdSeconds")}; s.keyboardModifiers=j.value("keyboardModifiers",0u); s.captureKeys=j.value("captureKeys",s.captureKeys); s.capturePads=j.value("capturePads",s.capturePads); if(!j.contains("capturePads")) for(auto& key:s.capturePads) if(key==s.gamepad) key=0; if(!j.contains("captureKeys")) for(auto& key:s.captureKeys) if(key==s.keyboard) key=0; s.autoKeyboard=j.value("autoKeyboard",0u);s.autoModifiers=j.value("autoModifiers",0u);s.autoGamepad=j.value("autoGamepad",0u);s.autoHoldSeconds=j.value("autoHoldSeconds",.8f); CheckInput(s); return s;
}
void WriteInputSettings(const InputSettings& s,const std::filesystem::path& path) {
    CheckInput(s);
    Json j={{"schema",1},{"keyboard",s.keyboard},{"gamepad",s.gamepad},{"holdSeconds",s.holdSeconds},{"captureKeys",s.captureKeys},{"capturePads",s.capturePads},{"keyboardModifiers",s.keyboardModifiers},{"autoKeyboard",s.autoKeyboard},{"autoModifiers",s.autoModifiers},{"autoGamepad",s.autoGamepad},{"autoHoldSeconds",s.autoHoldSeconds}};
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not save hotkeys");
}
void ValidateSettings(const StudioSettings& s) {
    if(s.startupTab.size()>240) throw std::runtime_error("Invalid startup tab");
    if(s.headSlots & ~0x1803u) throw std::runtime_error("Invalid head slots");
    if(s.language<0 || s.language>2) throw std::runtime_error("Invalid language");
    auto valid=[](float v,float low,float high){return std::isfinite(v) && v>=low && v<=high;};
    for(auto v:s.previewRect) if(!valid(v,0.f,1.f)) throw std::runtime_error("Invalid preview rectangle");
    if(s.previewRect[2]<.1f || s.previewRect[3]<.1f) throw std::runtime_error("Preview rectangle too small");
    if (!valid(s.distance,10,500) || !valid(s.height,-20,180) || !valid(s.orbit,-180,180) || !valid(s.pitch,-25,25) || !valid(s.lateral,-150,150) || !valid(s.elevation,-60,60) || !valid(s.fov,35,90) || s.columns<2 || s.columns>6)
        throw std::runtime_error("Invalid studio settings");
}
StudioSettings ReadStudioSettings(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path)>8192) throw std::runtime_error("Settings file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1) throw std::runtime_error("Unsupported settings schema");
    StudioSettings s{j.at("distance"),j.at("height"),j.at("orbit"),j.at("pitch"),j.at("fov"),j.value("columns",3)};
    s.showNames=j.value("showNames",true); s.showCounts=j.value("showCounts",true); s.language=j.value("language",0); s.allowFreeCamera=j.value("allowFreeCamera",false); s.addMissing=j.value("addMissing",true); s.preferEnchanted=j.value("preferEnchanted",true); s.portraitThumbnails=j.value("portraitThumbnails",false); s.cleanupCompatibility=j.value("cleanupCompatibility",false); s.allowCombatGallery=j.value("allowCombatGallery",false); s.compactGallery=j.value("compactGallery",false);
    s.headSlots=j.value("headSlots",0x1803u);
    s.startupTab=j.value("startupTab",std::string{});
    s.followerTargeting=j.value("followerTargeting",false);
    s.detachedPreview=j.value("detachedPreview",false); s.previewRect=j.value("previewRect",s.previewRect);
    s.lateral=j.value("lateral",0.f); s.elevation=j.value("elevation",0.f);
    ValidateSettings(s); return s;
}
void WriteStudioSettings(const StudioSettings& s,const std::filesystem::path& path) {
    ValidateSettings(s);
    Json j={{"schema",1},{"distance",s.distance},{"height",s.height},{"orbit",s.orbit},{"pitch",s.pitch},{"fov",s.fov},{"columns",s.columns},{"showNames",s.showNames},{"showCounts",s.showCounts},{"language",s.language},{"allowFreeCamera",s.allowFreeCamera},{"addMissing",s.addMissing},{"preferEnchanted",s.preferEnchanted},{"portraitThumbnails",s.portraitThumbnails},{"cleanupCompatibility",s.cleanupCompatibility},{"allowCombatGallery",s.allowCombatGallery},{"compactGallery",s.compactGallery},{"lateral",s.lateral},{"elevation",s.elevation},{"headSlots",s.headSlots},{"startupTab",s.startupTab},{"detachedPreview",s.detachedPreview},{"previewRect",s.previewRect},{"followerTargeting",s.followerTargeting}};
    std::filesystem::create_directories(path.parent_path());
    auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); out.exceptions(std::ios::badbit|std::ios::failbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not replace studio settings file");
}
CameraBank ReadCameraBank(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>16384) throw std::runtime_error("Camera preset file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1 || !j.at("slots").is_array() || j.at("slots").size()!=7) throw std::runtime_error("Invalid camera preset bank");
    CameraBank bank;
    if(j.contains("names")) {
        if(!j["names"].is_array() || j["names"].size()!=7) throw std::runtime_error("Invalid camera preset bank");
        bank.names=j["names"].get<std::array<std::string,7>>();
        for(const auto& name:bank.names) if(name.size()>255 || name.find('\0')!=std::string::npos) throw std::runtime_error("Invalid camera preset bank");
    }
    for(std::size_t n=0;n<7;++n) if(!j["slots"][n].is_null()) {
        const auto& c=j["slots"][n]; StudioSettings s;
        s.distance=c.at("distance"); s.height=c.at("height"); s.orbit=c.at("orbit"); s.pitch=c.at("pitch"); s.fov=c.at("fov");
        s.lateral=c.value("lateral",0.f); s.elevation=c.value("elevation",0.f);
        ValidateSettings(s); bank[n]=s;
    }
    return bank;
}
void WriteCameraBank(const CameraBank& bank,const std::filesystem::path& path) {
    for(const auto& name:bank.names) if(name.size()>255 || name.find('\0')!=std::string::npos) throw std::runtime_error("Invalid camera preset bank");
    Json j={{"schema",1},{"slots",Json::array()},{"names",bank.names}};
    for(const auto& slot:bank) {
        if(!slot) {j["slots"].push_back(nullptr); continue;}
        ValidateSettings(*slot); const auto& s=*slot;
        j["slots"].push_back({{"distance",s.distance},{"height",s.height},{"orbit",s.orbit},{"pitch",s.pitch},{"fov",s.fov},{"lateral",s.lateral},{"elevation",s.elevation}});
    }
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not save camera presets");
}
bool MovePhoto(std::vector<std::string>& order,const std::string& source,const std::string& target,bool after) {
    if(source==target || std::find(order.begin(),order.end(),source)==order.end() || std::find(order.begin(),order.end(),target)==order.end()) return false;
    std::erase(order,source);
    auto at=std::find(order.begin(),order.end(),target);
    order.insert(after?std::next(at):at,source); return true;
}
void CheckLibrary(const Library& l) {
    std::set<std::string> ordered;
    if(l.order.size()>10000) throw std::runtime_error("Too many ordered photos");
    for(const auto& photo:l.order) if(photo.empty() || photo.find_first_of("/\\:")!=std::string::npos || std::filesystem::path(photo).extension()!=".png" || !ordered.insert(photo).second) throw std::runtime_error("Invalid photo order");
    if(l.categories.size()>64 || l.labels.size()>10000) throw std::runtime_error("Library too large");
    std::set<std::string> ids;
    for(const auto& c:l.categories) {
        if(c.id.empty() || c.id.size()>80 || c.name.empty() || c.name.size()>240 || !ids.insert(c.id).second) throw std::runtime_error("Invalid category");
    }
    for(const auto& [photo,label]:l.labels) {
        if(photo.empty() || photo.find_first_of("/\\:")!=std::string::npos || std::filesystem::path(photo).extension()!=".png" || label.name.size()>240 || label.categories.size()>64) throw std::runtime_error("Invalid library entry");
        for(const auto& id:label.categories) if(!ids.contains(id)) throw std::runtime_error("Unknown category");
    }
}
Library ReadLibrary(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>4*1024*1024) throw std::runtime_error("Library file too large");
    std::ifstream in(path); Json j; in>>j;
    if(j.at("schema").get<int>()!=1) throw std::runtime_error("Unsupported library schema");
    Library l;
    l.order=j.value("order",std::vector<std::string>{});
    for(const auto& c:j.at("categories")) l.categories.push_back({c.at("id"),c.at("name")});
    for(const auto& [key,value]:j.at("labels").items()) l.labels[key]={value.at("name"),value.at("categories").get<std::vector<std::string>>(),value.at("deleted")};
    for(const auto& [key,value]:j.at("labels").items()) if(value.contains("exchangeSlots")) {
        const auto& m=value.at("exchangeSlots");
        if(!m.is_number_integer() || m.get<std::int64_t>()<0 || m.get<std::uint64_t>()>UINT32_MAX)
            throw std::runtime_error("Invalid exchange slots.");
        l.labels[key].exchangeSlots=m.get<std::uint32_t>();
        auto selected=value.value("exchangeSlotless",std::vector<std::string>{});
        if(selected.size()>64) throw std::runtime_error("Invalid exchange slots.");
        for(const auto& id:selected) if(id.empty() || id.size()>280) throw std::runtime_error("Invalid exchange slots.");
        l.labels[key].exchangeSlotless=std::move(selected);
    }
    for(const auto& [key,value]:j.at("labels").items()) if(value.contains("outfitItems")) {
        auto selected=value.at("outfitItems").get<std::vector<std::string>>();
        if(selected.size()>256) throw std::runtime_error("Invalid exchange slots.");
        for(const auto& id:selected) if(id.empty() || id.size()>280) throw std::runtime_error("Invalid exchange slots.");
        l.labels[key].outfitItems=std::move(selected);
    }
    CheckLibrary(l); return l;
}
// Diagnostic observers do not authorize deletion or alter the safety decision.
static std::string DiagnosticPath(const std::filesystem::path& path) {
    const auto utf8=path.generic_u8string(); return {utf8.begin(),utf8.end()};
}
static Json DescribeDeletionPath(const std::filesystem::path& path) {
    Json j; j["path"]=DiagnosticPath(path);
    std::error_code ec;
    const auto status=std::filesystem::symlink_status(path,ec);
    j["status_error"]=ec.value(); j["type"]=static_cast<int>(status.type());
    j["symlink"]=std::filesystem::is_symlink(status);
    auto canonical=std::filesystem::weakly_canonical(path,ec);
    j["canonical_error"]=ec.value(); if(!ec) j["canonical"]=DiagnosticPath(canonical);
    const DWORD attributes=GetFileAttributesW(path.c_str());
    j["attributes"]=attributes;
    if(attributes==INVALID_FILE_ATTRIBUTES) j["attributes_error"]=GetLastError();
    else j["reparse_point"]=(attributes&FILE_ATTRIBUTE_REPARSE_POINT)!=0;
    // Read-only handle metadata. A VFS may virtualize this observation too.
    HANDLE file=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if(file==INVALID_HANDLE_VALUE) j["handle_error"]=GetLastError();
    else {
        wchar_t buffer[32768];
        const DWORD length=GetFinalPathNameByHandleW(file,buffer,32768,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
        const DWORD error=length?0:GetLastError(); CloseHandle(file);
        if(length && length<32768) j["handle_path"]=DiagnosticPath(std::filesystem::path(std::wstring(buffer,length)));
        else {j["handle_path_error"]=error;j["handle_path_length"]=length;}
    }
    return j;
}
void DeleteTrashedFiles(const std::string& photo,const Library& library,const std::filesystem::path& root,DeletionTrace trace) {
    auto emit=[&](auto makeEvent) noexcept {
        if(!trace) return;
        try {auto event=makeEvent();event["photo"]=photo;trace(event.dump(-1,' ',false,Json::error_handler_t::replace));} catch(...) {}
    };
    auto describe=[&](const std::filesystem::path& path,const char* role) noexcept {
        if(!trace) return;
        try {auto event=DescribeDeletionPath(path);event["event"]="path";event["role"]=role;emit([&] {return std::move(event);});} catch(...) {}
    };
    std::string stage="library";
    std::filesystem::path candidate=root;
    if(trace) emit([&] {return Json{{"event","begin"},{"root",DiagnosticPath(root)}};});
    try {
        CheckLibrary(library);
        const auto label=library.labels.find(photo);
        if(label==library.labels.end() || !label->second.deleted) throw std::runtime_error("Only trashed presets can be deleted");
        stage="root";
        const auto base=std::filesystem::absolute(root).lexically_normal();
        describe(root,"root"); describe(root/"Presets","preset_folder"); describe(root/"Captures","capture_folder");
        std::vector<std::filesystem::path> files;
        auto noReparse=[&](const auto& path) {
            const DWORD attributes=GetFileAttributesW(path.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES) {
                const DWORD error=GetLastError();
                if(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND) return;
                throw std::filesystem::filesystem_error("Could not inspect deletion path",path,
                    std::error_code(static_cast<int>(error),std::system_category()));
            }
            if(attributes&FILE_ATTRIBUTE_REPARSE_POINT) {
                if(trace) emit([&] {return Json{{"event","rejected"},{"reason","reparse_point"},{"path",DiagnosticPath(path)}};});
                throw std::runtime_error("Unsafe deletion path");
            }
        };
        // The caller supplies the trusted root. Reject real links below it,
        // including junctions on Presets/Captures, without following their targets.
        for(const auto* folder:{"Presets","Captures"}) {
            candidate=root/folder; stage=std::string("safety_")+folder;
            noReparse(candidate);
        }
        auto safe=[&](const auto& path,const char* folder) {
            candidate=path; stage=std::string("safety_")+folder;
            describe(path,"candidate");
            // File symlinks and other reparse points remain forbidden.
            const bool symlink=std::filesystem::is_symlink(std::filesystem::symlink_status(path));
            if(symlink) {if(trace) emit([&] {return Json{{"event","rejected"},{"reason","candidate_symlink"},{"path",DiagnosticPath(path)}};});throw std::runtime_error("Unsafe deletion path");}
            noReparse(root/folder);
            noReparse(path);
            const auto actual=std::filesystem::absolute(path).lexically_normal().parent_path();
            const auto expected=base/folder;
            const bool matches=IsDirectDeletionChild(path,root/folder);
            if(trace) emit([&] {return Json{{"event","comparison"},{"basis","virtual_lexical"},{"path",DiagnosticPath(path)},{"actual_parent",DiagnosticPath(actual)},{"expected_parent",DiagnosticPath(expected)},{"matches",matches}};});
            if(!matches) {
                describe(expected,"resolved_expected_folder");
                if(trace) emit([&] {return Json{{"event","rejected"},{"reason","parent_mismatch"},{"path",DiagnosticPath(path)}};});
                throw std::runtime_error("Unsafe deletion path");
            }
        };
        const auto presets=root/"Presets";
        stage="enumerate_presets";candidate=presets;
        if(std::filesystem::exists(presets)) for(const auto& entry:std::filesystem::directory_iterator(presets)) {
            if(entry.path().extension()!=".json") continue;
            // Intentionally retain the existing check-before-match ordering.
            safe(entry.path(),"Presets");
            stage="read_preset";candidate=entry.path();
            const auto referenced=ReadPreset(entry.path()).photo;
            if(trace) emit([&] {return Json{{"event","preset_match"},{"path",DiagnosticPath(entry.path())},{"referenced_photo",referenced},{"matches",referenced==photo}};});
            if(referenced==photo) files.push_back(entry.path());
        }
        const auto image=root/"Captures"/photo;
        safe(image,"Captures"); files.push_back(image);
        // Only inspect edit records during an explicit permanent deletion.
        // Match both the backing preset filename and its photo identity.
        std::vector<std::string> presetNames;
        for(const auto& file:files) if(file.extension()==".json") presetNames.push_back(file.filename().string());
        for(const auto* folder:{"Presets/EditOriginals","Presets/EditBackups"}) {
            const auto directory=root/folder;
            noReparse(directory);
            if(!std::filesystem::exists(directory))continue;
            for(const auto& entry:std::filesystem::directory_iterator(directory)) {
                const auto name=entry.path().filename().string();
                const bool match=std::any_of(presetNames.begin(),presetNames.end(),[&](const auto& presetName){
                    if(name==presetName+".original")return std::string_view(folder)=="Presets/EditOriginals";
                    if(std::string_view(folder)!="Presets/EditBackups" || !name.starts_with(presetName+".") || !name.ends_with(".bak"))return false;
                    const auto token=name.substr(presetName.size()+1,name.size()-presetName.size()-5);
                    const auto dash=token.find('-');
                    return dash!=std::string::npos && dash>0 && dash+1<token.size() && token.find('-',dash+1)==std::string::npos && token.find_first_not_of("0123456789-")==std::string::npos;
                });
                if(!match)continue;
                safe(entry.path(),folder);
                if(ReadPreset(entry.path()).photo!=photo)throw std::runtime_error("Edit record does not match the deleted preset");
                files.insert(files.begin(),entry.path()); // Keep the preset available if edit-record deletion fails.
            }
        }
        stage="regular_file_check";
        for(const auto& file:files) {candidate=file;if(std::filesystem::exists(file) && !std::filesystem::is_regular_file(file)) throw std::runtime_error("Deletion target is not a regular file");}
        for(const auto& file:files) {
            candidate=file;stage="remove";
            if(trace) emit([&] {return Json{{"event",stage},{"path",DiagnosticPath(file)},{"exists",std::filesystem::exists(file)}};});
            const bool removed=std::filesystem::remove(file);
            if(trace) emit([&] {return Json{{"event","remove_result"},{"path",DiagnosticPath(file)},{"removed",removed}};});
        }
        if(trace) emit([&] {return Json{{"event","complete"},{"target_count",files.size()}};});
    } catch(const std::exception& e) {
        if(trace) {
            try {
                Json event={{"event","failed"},{"stage",stage},{"path",DiagnosticPath(candidate)},{"error",e.what()}};
                if(const auto* fs=dynamic_cast<const std::filesystem::filesystem_error*>(&e)) {
                    event["error_code"]=fs->code().value(); event["error_category"]=fs->code().category().name();
                }
                emit([&] {return std::move(event);});
            } catch(...) {}
        }
        throw;
    }
}
void WriteLibrary(const Library& l,const std::filesystem::path& path) {
    CheckLibrary(l);
    Json j={{"schema",1},{"categories",Json::array()},{"labels",Json::object()}};
    j["order"]=l.order;
    for(const auto& c:l.categories) j["categories"].push_back({{"id",c.id},{"name",c.name}});
    for(const auto& [key,value]:l.labels) j["labels"][key]={{"name",value.name},{"categories",value.categories},{"deleted",value.deleted}};
    for(const auto& [key,value]:l.labels) if(value.outfitItems) j["labels"][key]["outfitItems"]=*value.outfitItems;
    for(const auto& [key,value]:l.labels) if(value.exchangeSlots) {j["labels"][key]["exchangeSlots"]=*value.exchangeSlots;j["labels"][key]["exchangeSlotless"]=value.exchangeSlotless;}
    std::filesystem::create_directories(path.parent_path()); auto tmp=path; tmp+=".tmp";
    {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); out.exceptions(std::ios::failbit|std::ios::badbit); out<<j.dump(2); out.close();}
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Could not save library");
}
void Validate(const Preset& p) {
    if (p.name.empty() || p.name.size() > 240 || p.items.empty() || p.items.size() > 64)
        throw std::runtime_error("Invalid preset name or item count");
    if (p.photo.empty() || p.photo.find_first_of("/\\:") != std::string::npos || std::filesystem::path(p.photo).extension() != ".png")
        throw std::runtime_error("Invalid preset photo path");
    std::set<std::string> seen;
    for (const auto& i : p.items) {
        if((p.slotMask || p.accessories) && i.kind!="armor") throw std::runtime_error("Partial presets support armor slots only");
        if (i.plugin.empty() || i.plugin.size() > 260 || i.plugin.find_first_of("/\\:") != std::string::npos || !i.localID || i.localID > 0xFFFFFF)
            throw std::runtime_error("Invalid plugin or local FormID");
        if ((i.kind != "armor" && i.kind != "weapon" && i.kind != "ammo") || (i.hand != "" && i.hand != "left" && i.hand != "right") || (i.kind != "weapon" && !i.hand.empty()))
            throw std::runtime_error("Unsupported item kind or hand");
        if (!seen.insert(i.plugin + ":" + std::to_string(i.localID) + ":" + i.hand).second)
            throw std::runtime_error("Duplicate preset item");
    }
}
void WritePreset(const Preset& p, const std::filesystem::path& path) {
    Validate(p);
    Json j = {{"schema",p.accessories?4:(p.slotMask?2:1)},{"name",p.name},{"photo",p.photo},{"items",Json::array()}};
    if(p.slotMask || p.accessories) j["slotMask"]=p.slotMask;
    if(p.equipmentEdited) j["equipmentEdited"]=true;
    for (const auto& i : p.items) {
        Json item={{"plugin",i.plugin},{"localID",i.localID},{"name",i.name},{"kind",i.kind},{"hand",i.hand}};
        if(i.preferred.custom) item["preferredEnchanted"]={{"scope",i.preferred.scope},{"id",i.preferred.id}};
        if(i.preferred.custom && !i.preferred.signature.empty()) {
            item["preferredEnchanted"]["signatureVersion"]=1;
            item["preferredEnchanted"]["signature"]=i.preferred.signature;
        }
        j["items"].push_back(std::move(item));
    }
    std::filesystem::create_directories(path.parent_path());
    auto tmp = path; tmp += ".tmp";
    { std::ofstream out(tmp, std::ios::binary | std::ios::trunc); out.exceptions(std::ios::failbit | std::ios::badbit); out << j.dump(2); out.close(); }
    std::filesystem::rename(tmp, path);
}
Preset ReadPreset(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 256*1024) throw std::runtime_error("Preset too large");
    std::ifstream in(path, std::ios::binary); Json j; in >> j;
    const int schema=j.at("schema").get<int>();
    if (schema!=1 && schema!=2 && schema!=3 && schema!=4) throw std::runtime_error("Unsupported preset schema");
    Preset p{j.at("name").get<std::string>(),j.at("photo").get<std::string>(),{}};
    p.accessories=schema==3 || schema==4;
    p.equipmentEdited=j.value("equipmentEdited",false);
    if(schema>=2) {
        const auto mask=j.at("slotMask").get<std::int64_t>();
        if(mask<0 || (mask==0 && schema!=4) || mask>UINT32_MAX) throw std::runtime_error("Invalid partial preset slots");
        p.slotMask=static_cast<std::uint32_t>(mask);
    }
    for (const auto& i : j.at("items")) {
        const auto id = i.at("localID").get<std::int64_t>();
        if (id <= 0 || id > 0xFFFFFF) throw std::runtime_error("Invalid FormID");
        p.items.push_back({i.at("plugin"),static_cast<std::uint32_t>(id),i.at("name"),i.at("kind"),i.at("hand")});
        if(i.contains("preferredEnchanted")) {
            auto& pref=p.items.back().preferred; pref.custom=true;
            try {
                const auto& m=i.at("preferredEnchanted");
                if(!m.at("scope").is_number_unsigned() || !m.at("id").is_number_unsigned()) continue;
                const auto uid=m.at("id").get<std::uint64_t>();
                if(uid>65535) continue;
                pref.scope=m.at("scope").get<std::uint64_t>(); pref.id=static_cast<std::uint16_t>(uid);
                if(m.contains("signatureVersion") && m["signatureVersion"]==1 && m.contains("signature") && m["signature"].is_string()) {
                    const auto& signature=m["signature"].get_ref<const std::string&>();
                    if(signature.size()<=16384) pref.signature=signature;
                }
            } catch(const Json::exception&) {pref.scope=0; pref.id=0;}
        }
    }
    Validate(p); return p;
}
}




namespace Gallery {
namespace {
std::string EditFileBytes(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>256*1024) throw std::runtime_error("Preset too large");
    std::ifstream in(path,std::ios::binary);in.exceptions(std::ios::badbit);
    if(!in) throw std::runtime_error("Could not read preset for editing.");
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
}
PresetEditFile FindPresetForEditing(const std::filesystem::path& folder,const std::string& photo) {
    PresetEditFile found;
    for(const auto& entry:std::filesystem::directory_iterator(folder)) {
        if(!entry.is_regular_file() || entry.path().extension()!=".json") continue;
        Preset p;std::string content;
        try {content=EditFileBytes(entry.path());p=ReadPreset(entry.path());}catch(const std::exception&){continue;}
        if(p.photo!=photo) continue;
        if(EditFileBytes(entry.path())!=content) throw std::runtime_error("Preset changed. Reopen the editor.");
        if(!found.path.empty()) throw std::runtime_error("Multiple presets reference this photo. Editing was cancelled.");
        found={entry.path(),std::move(content),p};
    }
    if(found.path.empty()) throw std::runtime_error("Preset file was not found.");
    return found;
}
std::optional<PresetEditOrigin> ReadPresetEditOrigin(const std::filesystem::path& path,const std::string& photo) {
    const auto baseline=path.parent_path()/"EditOriginals"/(path.filename().string()+".original");
    if(std::filesystem::exists(baseline)) {
        auto p=ReadPreset(baseline);
        if(p.photo!=photo || p.slotMask || p.accessories) throw std::runtime_error("Invalid original equipment record.");
        const auto content=EditFileBytes(baseline);const auto j=Json::parse(content);
        return PresetEditOrigin{p,content,j.value("originFromLegacyBackup",false)};
    }
    const auto backups=path.parent_path()/"EditBackups";
    if(!std::filesystem::exists(backups))return {};
    std::vector<std::pair<std::pair<unsigned long long,unsigned long long>,std::filesystem::path>> candidates;
    const auto prefix=path.filename().string()+".";
    for(const auto& e:std::filesystem::directory_iterator(backups)) {
        const auto name=e.path().filename().string();
        if(!e.is_regular_file() || !name.starts_with(prefix) || !name.ends_with(".bak"))continue;
        const auto token=name.substr(prefix.size(),name.size()-prefix.size()-4);const auto dash=token.find('-');
        if(dash==std::string::npos || token.find_first_not_of("0123456789-")!=std::string::npos)continue;
        try{std::size_t a{},b{};const auto time=std::stoull(token.substr(0,dash),&a),seq=std::stoull(token.substr(dash+1),&b);
            if(a!=dash || b!=token.size()-dash-1)continue;candidates.push_back({{time,seq},e.path()});}catch(const std::exception&){}
    }
    if(candidates.empty())return {};
    std::sort(candidates.begin(),candidates.end());
    // Never silently skip a damaged oldest backup and call a later version the original.
    const auto& first=candidates.front().second;auto p=ReadPreset(first);
    if(p.photo!=photo || p.slotMask || p.accessories)throw std::runtime_error("Invalid original equipment record.");
    return PresetEditOrigin{p,EditFileBytes(first),true};
}
namespace {
bool SameEditedContents(const Preset& a,const Preset& b) {
    if(a.items.size()!=b.items.size())return false;
    for(const auto& i:a.items) {
        auto it=std::find_if(b.items.begin(),b.items.end(),[&](const auto& j){return SameBaseItem(i,j) && i.hand==j.hand;});
        if(it==b.items.end() || i.preferred.custom!=it->preferred.custom || i.preferred.id!=it->preferred.id || i.preferred.scope!=it->preferred.scope || i.preferred.signature!=it->preferred.signature)return false;
    }
    return true;
}
}
PresetEditSaved SaveEditedPreset(const PresetEditFile& original,const Preset& replacement) {
    if(replacement.photo!=original.original.photo || replacement.name!=original.original.name || replacement.slotMask || replacement.accessories)
        throw std::runtime_error("Preset identity changed. Editing was cancelled.");
    if(EditFileBytes(original.path)!=original.bytes) throw std::runtime_error("Preset changed. Reopen the editor.");
    static std::atomic<unsigned> serial{};
    const auto token=std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial);
    const auto baseline=original.path.parent_path()/"EditOriginals"/(original.path.filename().string()+".original");
    const auto staged=original.path.parent_path()/(original.path.filename().string()+"."+token+".edit");
    bool edited=false;
    try {
        Validate(replacement);
        const auto origin=ReadPresetEditOrigin(original.path,original.original.photo);
        if(!std::filesystem::exists(baseline)) {
            auto content=Json::parse(origin?origin->bytes:original.bytes);
            content["originFromLegacyBackup"]=origin.has_value();
            std::filesystem::create_directories(baseline.parent_path());
            auto temp=baseline;temp+=".tmp";
            {std::ofstream stream(temp,std::ios::binary);stream.exceptions(std::ios::failbit|std::ios::badbit);stream<<content.dump(2);stream.close();}
            std::filesystem::rename(temp,baseline);
        }
        auto saved=replacement;
        edited=!SameEditedContents(origin?origin->preset:original.original,saved);
        saved.equipmentEdited=edited;
        WritePreset(saved,staged); // validate and completely write before touching the original
        if(EditFileBytes(original.path)!=original.bytes) throw std::runtime_error("Preset changed. Reopen the editor.");
        if(!MoveFileExW(staged.c_str(),original.path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Could not save edited preset. Original was kept.");
    } catch(...) {
        std::error_code error;std::filesystem::remove(staged,error);auto tmp=staged;tmp+=".tmp";std::filesystem::remove(tmp,error);throw;
    }
    return {baseline,edited};
}
}
