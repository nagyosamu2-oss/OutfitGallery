#include "Presets.h"
#include "HoldButton.h"
#include "DiagnosticSettings.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <stdexcept>
int main() {
    using namespace Gallery;
    auto folder = std::filesystem::temp_directory_path() / ("OutfitGalleryTest-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        Preset p{"白いドレス","portrait-1.png",{{"Dress.esp",0xABC,"ドレス","armor",""},{"Sword.esl",0x801,"Sword","weapon","left"},{"Sword.esl",0x801,"Sword","weapon","right"}}};
        WritePreset(p,folder/"valid.json");
        auto q=ReadPreset(folder/"valid.json");
        if(q.items[0].preferred.custom) throw std::runtime_error("legacy preset acquired preference");
        auto enchanted=p; enchanted.items[0].preferred={true,0xFEDCBA9876543210ULL,65000,"stable-effect-signature"};
        WritePreset(enchanted,folder/"enchanted.json");
        auto remembered=ReadPreset(folder/"enchanted.json");
        if(!remembered.items[0].preferred.custom || remembered.items[0].preferred.scope!=enchanted.items[0].preferred.scope || remembered.items[0].preferred.id!=65000) throw std::runtime_error("preferred identity lost");
        if(remembered.items[0].preferred.signature!="stable-effect-signature") throw std::runtime_error("preferred signature lost");
        StudioSettings prefSettings;
        if(!prefSettings.preferEnchanted) throw std::runtime_error("preference not default ON");
        {std::ofstream out(folder/"legacy-preference.json"); out<<R"({"schema":1,"distance":220,"height":65,"orbit":0,"pitch":0,"fov":60})";}
        if(!ReadStudioSettings(folder/"legacy-preference.json").preferEnchanted) throw std::runtime_error("missing preference field not default ON");
        if(prefSettings.cleanupCompatibility || cleanupCompatibility.load() || DetailEnabled()) throw std::runtime_error("support options must default OFF");
        if(ReadStudioSettings(folder/"legacy-preference.json").cleanupCompatibility) throw std::runtime_error("legacy settings enabled compatibility");
        prefSettings.cleanupCompatibility=true; prefSettings.portraitThumbnails=true;
        detailedLogging=true;
        WriteStudioSettings(prefSettings,folder/"support.json");
        const auto support=ReadStudioSettings(folder/"support.json");
        if(!support.cleanupCompatibility || !support.portraitThumbnails) throw std::runtime_error("support setting did not persist");
        {std::ifstream in(folder/"support.json"); std::string bytes((std::istreambuf_iterator<char>(in)),{});
         if(bytes.find("detailedLogging")!=std::string::npos) throw std::runtime_error("diagnostics persisted across restart");}
        detailedLogging=false;
        prefSettings.cleanupCompatibility=false; WriteStudioSettings(prefSettings,folder/"support.json");
        if(ReadStudioSettings(folder/"support.json").cleanupCompatibility) throw std::runtime_error("compatibility OFF did not persist");
        prefSettings.preferEnchanted=true; WriteStudioSettings(prefSettings,folder/"prefer.json");
        if(!ReadStudioSettings(folder/"prefer.json").preferEnchanted) throw std::runtime_error("preference not saved");
        prefSettings.preferEnchanted=false; WriteStudioSettings(prefSettings,folder/"prefer.json");
        if(ReadStudioSettings(folder/"prefer.json").preferEnchanted || ReadPreset(folder/"enchanted.json").items[0].preferred.id!=65000) throw std::runtime_error("OFF erased identity");
        if(q.name!=p.name || q.items.size()!=3 || q.items[0].localID!=0xABC || q.items[1].hand!="left" || q.items[2].plugin!="Sword.esl") throw std::runtime_error("round trip failed");
        auto legacy=q;
        legacy.items.push_back({"MissingArrows.esp",0x800,"Arrows","ammo",""});
        const auto clothing=ClothingPreset(legacy);
        if(clothing.items.size()!=1 || clothing.items[0].kind!="armor" || legacy.items.size()!=4 || clothing.photo!=legacy.photo) throw std::runtime_error("Legacy weapons/ammo were not excluded without changing saved data");
        if(!SameEquipment(clothing,ClothingPreset(p))) throw std::runtime_error("Legacy hand equipment affects clothing verification");
        auto rejects=[&](Preset bad) { try { WritePreset(bad,folder/"bad.json"); } catch(const std::exception&) { return; } throw std::runtime_error("accepted invalid preset"); };
        if(q.slotMask) throw std::runtime_error("Legacy outfit became partial");
        if(q.accessories) throw std::runtime_error("Legacy outfit became accessory set");
        Preset accessories{"Ring and necklace","accessories.png",{{"Jewelry.esp",0x123,"Ring","armor",""},{"Jewelry.esp",0x124,"Necklace","armor",""}},0x60,true};
        WritePreset(accessories,folder/"accessories.json");
        const auto accessoryCopy=ReadPreset(folder/"accessories.json");
        if(!accessoryCopy.accessories || accessoryCopy.slotMask!=0x60 || !SameEquipment(accessories,accessoryCopy)) throw std::runtime_error("Accessory set roundtrip failed");
        if(!SameEquipment(accessories,ClothingPreset(accessories)) || !ClothingPreset(accessories).accessories || ClothingPreset(accessories).slotMask!=0x60) throw std::runtime_error("Clothing filter changed accessory scope");
        for(const auto& category:std::vector<std::string>{"","favorites","#head","#legacy"}) if(CollectionFits(2,category,true)) throw std::runtime_error("Accessory photo leaked to other collections");
        if(!CollectionFits(2,"#accessories",true) || !CollectionFits(2,"#trash",true) || CollectionFits(2,"#accessories",false)) throw std::runtime_error("Accessory isolation failed");
        if(AccessoryItemInScope(4,0,false) || AccessoryItemInScope(0,0,false) || !AccessoryItemInScope(0,0,true)) throw std::runtime_error("Slotless accessory must preserve unrelated equipment");
        if(!AccessoryItemInScope(0x60,0x20,false) || AccessoryItemInScope(4,0x20,false) || !AccessoryItemInScope(0,0x20,true)) throw std::runtime_error("Mixed accessory scope failed");
        {std::ofstream old(folder/"oldaccessory.json");old<<R"({"schema":3,"name":"Old","photo":"old.png","slotMask":32,"items":[{"plugin":"Jewelry.esp","localID":291,"name":"Ring","kind":"armor","hand":""}]})";}
        if(!ReadPreset(folder/"oldaccessory.json").accessories || ReadPreset(folder/"oldaccessory.json").slotMask!=32) throw std::runtime_error("Legacy accessory changed");
        auto invalidAccessories=accessories;
        auto slotless=accessories;slotless.slotMask=0;
        WritePreset(slotless,folder/"slotless.json");
        const auto slotlessCopy=ReadPreset(folder/"slotless.json");
        if(!slotlessCopy.accessories || slotlessCopy.slotMask || !SameEquipment(slotless,slotlessCopy)) throw std::runtime_error("Slotless accessory persistence failed");
        invalidAccessories=accessories; invalidAccessories.items[0].kind="weapon"; rejects(invalidAccessories);
        // A ring overlaps the target; body armor sharing its slot is replaced.
        // Incoming multi-slot armor must still fit; unrelated armor is outside scope.
        if(!SlotFits(0x40,0x60) || !SlotIntersects(0x44,0x60) || SlotFits(0x44,0x60) || SlotIntersects(4,0x60)) throw std::runtime_error("Accessory conflict protection failed");
        if(CollectionFits(0x1803,"") || CollectionFits(2,"favorites") || CollectionFits(0,"#head") || CollectionFits(4,"#head") || !CollectionFits(2,"#head") || !CollectionFits(4,"#legacy") || !CollectionFits(2,"#trash")) throw std::runtime_error("Independent head collection leaked");
        Preset part{"Head","head.png",{{"Hair.esp",0x123,"Hair","armor",""}},0x1803};
        WritePreset(part,folder/"partial.json");
        if(ReadPreset(folder/"partial.json").slotMask!=0x1803) throw std::runtime_error("Partial scope lost");
        part.slotMask=0x80000000u; WritePreset(part,folder/"slot61.json");
        if(ReadPreset(folder/"slot61.json").slotMask!=0x80000000u) throw std::runtime_error("High slot bit lost");
        auto invalidPart=p; invalidPart.slotMask=0x1803; rejects(invalidPart);
        if(!SlotIntersects(0x1802,0x1803) || !SlotFits(0x1802,0x1803) || SlotIntersects(4,0x1803) || SlotFits(6,0x1803) || SlotFits(0,0x1803)) throw std::runtime_error("Partial scope affects outside/body slots");
        {std::ofstream out(folder/"empty-scope.json"); out<<R"({"schema":2,"name":"bad","photo":"bad.png","slotMask":0,"items":[]})";}
        bool invalidScope=false; try{ReadPreset(folder/"empty-scope.json");}catch(const std::exception&){invalidScope=true;}
        if(!invalidScope) throw std::runtime_error("Empty partial scope accepted as full outfit");
        auto bad=p; bad.photo="../outside.png"; rejects(bad);
        bad=p; bad.items[0].localID=0xFE123ABC; rejects(bad);
        bad=p; bad.items.push_back(bad.items[0]); rejects(bad);
        bad=p; bad.items[0].hand="left"; rejects(bad);
        {std::ofstream out(folder/"broken.json"); out << "{broken";}
        bool caught=false; try { ReadPreset(folder/"broken.json"); } catch(const std::exception&) { caught=true; }
        if(!caught) throw std::runtime_error("accepted broken JSON");
        StudioSettings settings{219.916f,64.189f,-6.545f,.350f,60.846f,4};
        auto lowCamera=settings; lowCamera.height=-20;
        WriteStudioSettings(lowCamera,folder/"low-camera.json");
        CameraBank lowBank{}; lowBank[2]=lowCamera;
        WriteCameraBank(lowBank,folder/"low-bank.json");
        if(ReadStudioSettings(folder/"low-camera.json").height!=-20 || ReadCameraBank(folder/"low-bank.json")[2]->height!=-20) throw std::runtime_error("Lower camera range did not persist");
        lowCamera.height=-21;
        bool lowRejected=false;
        try {WriteStudioSettings(lowCamera,folder/"low-camera.json");} catch(const std::exception&) {lowRejected=true;}
        if(!lowRejected || ReadStudioSettings(folder/"low-camera.json").height!=-20) throw std::runtime_error("Invalid camera height overwrote valid settings");
        if(ReadStudioSettings(folder/"low-camera.json").detachedPreview) throw std::runtime_error("Preview must default off");
        settings.detachedPreview=true; settings.previewRect={.25f,.1f,.4f,.7f};
        WriteStudioSettings(settings,folder/"preview.json");
        const auto previewSettings=ReadStudioSettings(folder/"preview.json");
        if(!previewSettings.detachedPreview || previewSettings.previewRect!=settings.previewRect) throw std::runtime_error("Preview settings not persisted");
        auto badPreview=settings; badPreview.previewRect[2]=0;
        bool previewRejected=false;
        try {WriteStudioSettings(badPreview,folder/"preview.json");} catch(const std::exception&) {previewRejected=true;}
        if(!previewRejected || ReadStudioSettings(folder/"preview.json").previewRect!=settings.previewRect) throw std::runtime_error("Invalid preview overwrote settings");
        if(ReadStudioSettings(folder/"low-camera.json").followerTargeting) throw std::runtime_error("Follower targeting must default off");
        settings.followerTargeting=true;
        WriteStudioSettings(settings,folder/"follower.json");
        if(!ReadStudioSettings(folder/"follower.json").followerTargeting || ReadStudioSettings(folder/"follower.json").previewRect!=settings.previewRect) throw std::runtime_error("Follower settings damaged preview settings");
        settings.startupTab="custom-7";
        WriteStudioSettings(settings,folder/"opening-settings.json");
        if(ReadStudioSettings(folder/"opening-settings.json").startupTab!="custom-7") throw std::runtime_error("Opening tab not persisted");
        Library openingLibrary; openingLibrary.categories={{"favorites","Favorites"},{"custom-7","Renamed category"}};
        if(ResolveStartupTab("custom-7",openingLibrary)!="custom-7") throw std::runtime_error("Rename changed opening tab identity");
        if(ResolveStartupTab("deleted-category",openingLibrary)!="favorites") throw std::runtime_error("Missing opening tab fallback failed");
        if(ResolveStartupTab("",openingLibrary)!="" || ResolveStartupTab("#head",openingLibrary)!="#head" || ResolveStartupTab("#accessories",openingLibrary)!="#accessories") throw std::runtime_error("Built-in opening tab failed");
        openingLibrary.categories.clear();
        if(ResolveStartupTab("deleted-category",openingLibrary)!="") throw std::runtime_error("All fallback failed");
        {std::ofstream oldSettings(folder/"old-opening.json"); oldSettings << R"({"schema":1,"distance":220,"height":65,"orbit":0,"pitch":0,"fov":60})";}
        if(!StudioSettings{}.startupTab.empty() || !ReadStudioSettings(folder/"old-opening.json").startupTab.empty()) throw std::runtime_error("Unset opening tab must default to All");
        WriteStudioSettings(settings,folder/"settings.json");
        settings.headSlots=2;
        WriteStudioSettings(settings,folder/"head-settings.json");
        if(ReadStudioSettings(folder/"head-settings.json").headSlots!=2) throw std::runtime_error("Head slot preference lost");
        settings.columns=5; settings.distance=300; settings.showNames=false; settings.showCounts=false;
        WriteStudioSettings(settings,folder/"settings.json");
        const auto restored=ReadStudioSettings(folder/"settings.json");
        if(restored.distance!=300 || restored.height!=settings.height || restored.orbit!=settings.orbit || restored.pitch!=settings.pitch || restored.fov!=settings.fov || restored.columns!=5) throw std::runtime_error("settings replacement/roundtrip failed");
        caught=false; settings.fov=0;
        try { WriteStudioSettings(settings,folder/"settings.json"); } catch(const std::exception&) { caught=true; }
        if(!caught || ReadStudioSettings(folder/"settings.json").fov!=restored.fov) throw std::runtime_error("invalid settings overwrote good settings");
        if(restored.showNames || restored.showCounts) throw std::runtime_error("display preferences lost");
        Library lib{{{"one","普段着"},{"two","Favorites"}},{{"portrait-1.png",{"新しい名前",{"one","two"},false}}}};
        WriteLibrary(lib,folder/"library.json");
        lib.categories[0].name="Renamed tab"; lib.labels["portrait-1.png"].deleted=true;
        WriteLibrary(lib,folder/"library.json");
        auto read=ReadLibrary(folder/"library.json");
        std::vector<std::string> order{"a.png","b.png","c.png","d.png"};
        if(!MovePhoto(order,"a.png","c.png",true) || order!=std::vector<std::string>{"b.png","c.png","a.png","d.png"}) throw std::runtime_error("forward photo move failed");
        if(!MovePhoto(order,"d.png","b.png",false) || order!=std::vector<std::string>{"d.png","b.png","c.png","a.png"}) throw std::runtime_error("backward photo move failed");
        if(MovePhoto(order,"d.png","d.png",true) || MovePhoto(order,"missing.png","b.png",false)) throw std::runtime_error("invalid photo move accepted");
        read.order=order; WriteLibrary(read,folder/"library.json");
        if(ReadLibrary(folder/"library.json").order!=order) throw std::runtime_error("photo order persistence failed");
        auto duplicateOrder=read; duplicateOrder.order.push_back("a.png"); caught=false;
        try {WriteLibrary(duplicateOrder,folder/"library.json");}catch(const std::exception&){caught=true;}
        if(!caught || ReadLibrary(folder/"library.json").order!=order) throw std::runtime_error("invalid order overwrote library");
        if(read.categories[0].name!="Renamed tab" || read.labels.at("portrait-1.png").categories.size()!=2 || !read.labels.at("portrait-1.png").deleted || read.labels.at("portrait-1.png").name!="新しい名前") throw std::runtime_error("library rename/trash roundtrip failed");
        read.labels["portrait-1.png"].deleted=false; WriteLibrary(read,folder/"library.json");
        if(ReadLibrary(folder/"library.json").labels.at("portrait-1.png").deleted) throw std::runtime_error("trash restore failed");
        auto invalid=read; invalid.labels["portrait-1.png"].categories.push_back("missing"); caught=false;
        try {WriteLibrary(invalid,folder/"library.json");}catch(const std::exception&){caught=true;}
        if(!caught || ReadLibrary(folder/"library.json").labels.at("portrait-1.png").categories.size()!=2) throw std::runtime_error("invalid library overwrote valid data");
        {
            InputSettings s;s.autoKeyboard=66;s.autoModifiers=1;s.autoGamepad=16;s.autoHoldSeconds=1.5f;
            WriteInputSettings(s,folder/"auto-hotkeys.json");auto r=ReadInputSettings(folder/"auto-hotkeys.json");
            if(r.autoKeyboard!=66 || r.autoModifiers!=1 || r.autoGamepad!=16 || r.autoHoldSeconds!=1.5f || r.keyboard!=66 || r.captureKeys!=s.captureKeys) throw std::runtime_error("auto bindings roundtrip");
            for(unsigned mods=0;mods<8;++mods) {s.autoKeyboard=88;s.autoModifiers=mods;WriteInputSettings(s,folder/"auto-hotkeys.json");if(ReadInputSettings(folder/"auto-hotkeys.json").autoModifiers!=mods)throw std::runtime_error("auto modifiers lost");}
            for(unsigned invalid=0;invalid<7;++invalid) {
                auto bad=s;
                if(invalid==0){bad.autoKeyboard=bad.keyboard;bad.autoModifiers=bad.keyboardModifiers;}
                if(invalid==1)bad.autoGamepad=bad.gamepad;
                if(invalid==2)bad.autoKeyboard=bad.captureKeys[0];
                if(invalid==3)bad.autoGamepad=bad.capturePads[0];
                if(invalid==4)bad.autoModifiers=8;
                if(invalid==5)bad.autoHoldSeconds=0;
                if(invalid==6)bad.autoGamepad=3;
                bool caught=false;try{WriteInputSettings(bad,folder/"auto-hotkeys.json");}catch(const std::exception&){caught=true;}
                if(!caught)throw std::runtime_error("invalid auto shortcut accepted");
            }
            {std::ofstream out(folder/"legacy-auto-hotkeys.json");out<<R"({"schema":1,"keyboard":66,"gamepad":32,"holdSeconds":0.8})";}
            auto old=ReadInputSettings(folder/"legacy-auto-hotkeys.json");
            if(old.autoKeyboard || old.autoModifiers || old.autoGamepad || old.autoHoldSeconds!=.8f)throw std::runtime_error("legacy shortcuts not disabled");
            s.autoKeyboard=0;s.autoModifiers=0;s.autoGamepad=0;
            WriteInputSettings(s,folder/"auto-hotkeys.json");r=ReadInputSettings(folder/"auto-hotkeys.json");
            if(r.autoKeyboard || r.autoGamepad)throw std::runtime_error("auto shortcuts cannot disable");
        }
        WriteInputSettings({65,128,1.2f},folder/"hotkeys.json");
        auto keys=ReadInputSettings(folder/"hotkeys.json");
        if(keys.keyboard!=65 || keys.gamepad!=128 || keys.holdSeconds!=1.2f) throw std::runtime_error("hotkeys roundtrip failed");
        WriteInputSettings({66,0,.8f},folder/"hotkeys.json");
        if(ReadInputSettings(folder/"hotkeys.json").gamepad!=0) throw std::runtime_error("disable pad failed");
        InputSettings customKeys; customKeys.captureKeys={88,0,89};customKeys.capturePads={64,32768,16384};
        WriteInputSettings(customKeys,folder/"capture-keys.json");
        const auto keysAgain=ReadInputSettings(folder/"capture-keys.json");
        if(keysAgain.captureKeys!=customKeys.captureKeys || keysAgain.capturePads!=customKeys.capturePads) throw std::runtime_error("Capture bindings lost");
        customKeys.capturePads[0]=9; bool reservedRejected=false;
        try{WriteInputSettings(customKeys,folder/"capture-keys.json");}catch(const std::exception&){reservedRejected=true;}
        if(!reservedRejected) throw std::runtime_error("Navigation capture conflict allowed");
        {std::ofstream out(folder/"old-keys.json");out<<R"({"schema":1,"keyboard":66,"gamepad":64,"holdSeconds":0.8})";}
        if(ReadInputSettings(folder/"old-keys.json").capturePads[2]!=0) throw std::runtime_error("Old menu binding migration conflict");
        if(ReadInputSettings(folder/"old-keys.json").keyboardModifiers!=0) throw std::runtime_error("Legacy modifiers changed");
        for(unsigned modifiers=0;modifiers<8;++modifiers) {
            InputSettings chord; chord.keyboardModifiers=modifiers;
            WriteInputSettings(chord,folder/"chord.json");const auto loaded=ReadInputSettings(folder/"chord.json");
            if(loaded.keyboardModifiers!=modifiers || loaded.keyboard!=chord.keyboard || loaded.captureKeys!=chord.captureKeys) throw std::runtime_error("Chord settings lost");
        }
        for(const auto* bad:{"-1","8","true","1.5","4294967296"}) {
            {std::ofstream out(folder/"invalid-chord.json");out<<"{\"schema\":1,\"keyboard\":66,\"gamepad\":0,\"holdSeconds\":0.8,\"keyboardModifiers\":"<<bad<<"}";}
            bool rejected=false;try{(void)ReadInputSettings(folder/"invalid-chord.json");}catch(const std::exception&){rejected=true;}
            if(!rejected) throw std::runtime_error("Invalid chord accepted");
        }
        auto closeCamera=StudioSettings{};closeCamera.distance=10;WriteStudioSettings(closeCamera,folder/"close-camera.json");
        if(ReadStudioSettings(folder/"close-camera.json").distance!=10) throw std::runtime_error("Close camera not persisted");
        settings.fov=60; settings.language=0; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").language!=0) throw std::runtime_error("English preference lost");
        settings.language=1; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").language!=1) throw std::runtime_error("Japanese preference lost");
        settings.language=2; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").language!=2) throw std::runtime_error("External language preference lost");
        if(StudioSettings{}.allowCombatGallery || ReadStudioSettings(folder/"legacy-preference.json").allowCombatGallery) throw std::runtime_error("combat entry must default OFF");
        settings.allowCombatGallery=true;WriteStudioSettings(settings,folder/"combat-entry.json");
        if(!ReadStudioSettings(folder/"combat-entry.json").allowCombatGallery) throw std::runtime_error("combat entry ON lost");
        settings.allowCombatGallery=false;WriteStudioSettings(settings,folder/"combat-entry.json");
        if(ReadStudioSettings(folder/"combat-entry.json").allowCombatGallery) throw std::runtime_error("combat entry OFF lost");
        settings.allowFreeCamera=true; WriteStudioSettings(settings,folder/"settings.json");
        if(!ReadStudioSettings(folder/"settings.json").allowFreeCamera) throw std::runtime_error("free camera preference lost");
        settings.addMissing=true; WriteStudioSettings(settings,folder/"settings.json");
        if(!ReadStudioSettings(folder/"settings.json").addMissing) throw std::runtime_error("add missing enabled preference lost");
        settings.addMissing=false; WriteStudioSettings(settings,folder/"settings.json");
        if(ReadStudioSettings(folder/"settings.json").addMissing) throw std::runtime_error("add missing disabled preference lost");
        {std::ofstream out(folder/"legacy-settings.json"); out << R"({"schema":1,"distance":220,"height":65,"orbit":0,"pitch":0,"fov":60})";}
        if(!ReadStudioSettings(folder/"legacy-settings.json").addMissing) throw std::runtime_error("missing addMissing setting should default to enabled");
        if(ReadStudioSettings(folder/"legacy-settings.json").portraitThumbnails) throw std::runtime_error("legacy thumbnail mode changed");
        settings.portraitThumbnails=true; WriteStudioSettings(settings,folder/"portrait.json");
        if(!ReadStudioSettings(folder/"portrait.json").portraitThumbnails) throw std::runtime_error("portrait mode lost");
        settings.portraitThumbnails=false; WriteStudioSettings(settings,folder/"portrait.json");
        if(ReadStudioSettings(folder/"portrait.json").portraitThumbnails) throw std::runtime_error("normal mode lost");
        CameraBank bank{}; bank[0]=settings; bank[6]=settings; bank[6]->height=140;
        settings.lateral=42; settings.elevation=-35;
        WriteStudioSettings(settings,folder/"settings.json");
        const auto shifted=ReadStudioSettings(folder/"settings.json");
        if(shifted.lateral!=42 || shifted.elevation!=-35) throw std::runtime_error("camera movement settings lost");
        const auto legacyCamera=ReadStudioSettings(folder/"legacy-settings.json");
        if(legacyCamera.lateral!=0 || legacyCamera.elevation!=0) throw std::runtime_error("legacy composition changed");
        bank[6]->lateral=-55; bank[6]->elevation=40;
        WriteCameraBank(bank,folder/"camera.json");
        auto cameras=ReadCameraBank(folder/"camera.json");
        if(cameras[6]->lateral!=-55 || cameras[6]->elevation!=40) throw std::runtime_error("camera movement preset lost");
        if(!cameras[0] || cameras[1] || !cameras[6] || cameras[6]->height!=140 || cameras[0]->height!=settings.height) throw std::runtime_error("camera slots roundtrip failed");
        cameras[0]->distance=350; WriteCameraBank(cameras,folder/"camera.json");
        auto changed=ReadCameraBank(folder/"camera.json");
        if(changed[0]->distance!=350 || changed[6]->height!=140 || changed[3]) throw std::runtime_error("camera slot replacement changed other slots");
        cameras[6]->fov=0; caught=false;
        try {WriteCameraBank(cameras,folder/"camera.json");}catch(const std::exception&){caught=true;}
        if(!caught || ReadCameraBank(folder/"camera.json")[6]->fov!=settings.fov) throw std::runtime_error("invalid camera overwrote valid bank");
        auto independent=ReadPreset(folder/"valid.json");
        independent.name="A different display name";
        std::swap(independent.items[0],independent.items[2]);
        if(!SameEquipment(p,independent)) throw std::runtime_error("equipment comparison depends on name or order");
        independent.items[0].hand="left";
        if(SameEquipment(p,independent)) throw std::runtime_error("equipment comparison missed wrong hand");
        // The library owns the editable scope, never the original preset file.
        Preset mixed{"Mixed","mixed.png",{{"Gear.esp",1,"Clothes","armor",""},{"Gear.esp",2,"Slotless ring","armor",""},{"Gear.esp",3,"Other slotless","armor",""}},4,true};
        mixed.exchangeSlots=0;mixed.exchangeSlotless={ExchangeItemKey(mixed.items[1])};
        const auto onlyRing=FilterExchangeSlots(mixed,{4,0,0});
        if(!onlyRing.accessories || onlyRing.slotMask || onlyRing.items.size()!=1 || onlyRing.items[0].localID!=2) throw std::runtime_error("Slotless-only selection lost or became full outfit");
        if(AccessoryItemInScope(4,onlyRing.slotMask,false) || AccessoryItemInScope(0,onlyRing.slotMask,false)) throw std::runtime_error("Unselected equipment targeted");
        mixed.exchangeSlots=4;
        if(FilterExchangeSlots(mixed,{4,0,0}).items.size()!=2) throw std::runtime_error("Mixed slot and item selection failed");
        mixed.exchangeSlots=0;mixed.exchangeSlotless.clear();
        if(!FilterExchangeSlots(mixed,{4,0,0}).items.empty()) throw std::runtime_error("Clear all must stay empty");
        Library selectedItems;selectedItems.labels["mixed.png"].exchangeSlots=0;
        selectedItems.labels["mixed.png"].exchangeSlotless={"Gear.esp:2"};
        WriteLibrary(selectedItems,folder/"selected-items.json");
        const auto selectedRead=ReadLibrary(folder/"selected-items.json");
        if(selectedRead.labels.at("mixed.png").exchangeSlots!=0 || selectedRead.labels.at("mixed.png").exchangeSlotless!=std::vector<std::string>{"Gear.esp:2"}) throw std::runtime_error("Slotless selection persistence failed");
        Library itemChoice; itemChoice.labels["full.png"].outfitItems=std::vector<std::string>{"Gear.esp:2"};
        itemChoice.labels["empty.png"].outfitItems=std::vector<std::string>{};
        WriteLibrary(itemChoice,folder/"item-choice.json");
        const auto choiceRead=ReadLibrary(folder/"item-choice.json");
        if(choiceRead.labels.at("full.png").outfitItems!=itemChoice.labels.at("full.png").outfitItems || !choiceRead.labels.at("empty.png").outfitItems || !choiceRead.labels.at("empty.png").outfitItems->empty()) throw std::runtime_error("Full item selection persistence failed");
        auto full=mixed;full.slotMask=0;full.accessories=false;full.outfitItems=std::vector<std::string>{"Gear.esp:2"};
        const auto selectedFull=FilterExchangeSlots(full,{});
        if(selectedFull.slotMask || selectedFull.accessories || selectedFull.items.size()!=1 || selectedFull.items[0].localID!=2 || full.items.size()!=3) throw std::runtime_error("Full selection lost replacement mode or changed source");
        full.outfitItems=std::vector<std::string>{"Gear.esp:1"};
        if(FilterExchangeSlots(full,{}).items[0].localID!=1) throw std::runtime_error("Item selection requires slot resolution");
        Library scopes; scopes.labels["portrait-1.png"].exchangeSlots=0;
        WriteLibrary(scopes,folder/"scopes.json");
        auto scopeLoaded=ReadLibrary(folder/"scopes.json");
        if(!scopeLoaded.labels.at("portrait-1.png").exchangeSlots || *scopeLoaded.labels.at("portrait-1.png").exchangeSlots!=0) throw std::runtime_error("zero override lost");
        scopes.labels["portrait-1.png"].exchangeSlots=0x80000000u;
        WriteLibrary(scopes,folder/"scopes.json");
        if(ReadLibrary(folder/"scopes.json").labels.at("portrait-1.png").exchangeSlots!=0x80000000u) throw std::runtime_error("slot 61 lost");
        scopes.labels["portrait-1.png"].exchangeSlots.reset();
        WriteLibrary(scopes,folder/"scopes.json");
        if(ReadLibrary(folder/"scopes.json").labels.at("portrait-1.png").exchangeSlots) throw std::runtime_error("reset override failed");
        {std::ofstream out(folder/"scopes-bad.json"); out<<R"({"schema":1,"categories":[],"labels":{"x.png":{"name":"","categories":[],"deleted":false,"exchangeSlots":-1}}})";}
        caught=false;try{ReadLibrary(folder/"scopes-bad.json");}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("negative scope accepted");
        Preset partial{"parts","parts.png",{{"A.esp",1,"wig","armor",""},{"A.esp",2,"crown","armor",""},{"A.esp",3,"dress","armor",""}},0x1807,true};
        partial.items[0].preferred={true,123,5,"enchanted-wig"};
        const std::vector<std::uint32_t> masks{0x802,0x1000,4};
        partial.exchangeSlots=0x802;
        auto filtered=FilterExchangeSlots(partial,masks);
        if(filtered.items.size()!=1 || filtered.items[0].name!="wig" || filtered.items[0].preferred.signature!="enchanted-wig" || filtered.slotMask!=0x802 || partial.items.size()!=3) throw std::runtime_error("scope selection changed source or enchantment");
        partial.exchangeSlots=0; filtered=FilterExchangeSlots(partial,masks);
        if(!filtered.items.empty() || !filtered.exchangeSlots || *filtered.exchangeSlots) throw std::runtime_error("empty scope not explicit");
        partial.exchangeSlots=2; caught=false;try{FilterExchangeSlots(partial,masks);}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("split multi-slot item accepted");
        partial.exchangeSlots=0x80000000u; caught=false;try{FilterExchangeSlots(partial,masks);}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("out-of-scope selection accepted");
        partial.exchangeSlots.reset();
        if(FilterExchangeSlots(partial,{}).items.size()!=3) throw std::runtime_error("legacy scope changed");
        HoldButton hold;
        auto deletionRoot=folder/"deletion";
        auto trashPreset=p; trashPreset.photo="trash.png";
        WritePreset(trashPreset,deletionRoot/"Presets"/"record.json");
        std::filesystem::create_directories(deletionRoot/"Captures");
        {std::ofstream out(deletionRoot/"Captures"/"trash.png"); out<<"image";}
        auto keepPreset=p; keepPreset.photo="keep.png";
        WritePreset(keepPreset,deletionRoot/"Presets"/"keep.json");
        {std::ofstream out(deletionRoot/"Captures"/"keep.png"); out<<"keep";}
        Library trashLib; trashLib.labels["trash.png"].deleted=false;
        caught=false; try {DeleteTrashedFiles("trash.png",trashLib,deletionRoot);}catch(const std::exception&){caught=true;}
        if(!caught || !std::filesystem::exists(deletionRoot/"Presets"/"record.json")) throw std::runtime_error("non-trash deletion permitted");
        trashLib.labels["trash.png"].deleted=true;
        const auto originFile=deletionRoot/"Presets/EditOriginals/record.json.original";
        const auto historyFile=deletionRoot/"Presets/EditBackups/record.json.123-1.bak";
        const auto keepHistory=deletionRoot/"Presets/EditBackups/keep.json.123-1.bak";
        WritePreset(keepPreset,originFile);
        WritePreset(trashPreset,historyFile);
        WritePreset(keepPreset,keepHistory);
        caught=false;try{DeleteTrashedFiles("trash.png",trashLib,deletionRoot);}catch(const std::exception&){caught=true;}
        if(!caught || !std::filesystem::exists(historyFile) || !std::filesystem::exists(deletionRoot/"Presets/record.json"))throw std::runtime_error("mismatched origin must reject before deleting anything");
        WritePreset(trashPreset,originFile);
        DeleteTrashedFiles("trash.png",trashLib,deletionRoot);
        if(std::filesystem::exists(originFile) || std::filesystem::exists(historyFile) || !std::filesystem::exists(keepHistory))throw std::runtime_error("edit record cleanup did not isolate deleted preset");
        if(!std::filesystem::exists(deletionRoot/"Presets"/"keep.json") || !std::filesystem::exists(deletionRoot/"Captures"/"keep.png")) throw std::runtime_error("unrelated files removed");
        if(std::filesystem::exists(deletionRoot/"Captures"/"trash.png") || std::filesystem::exists(deletionRoot/"Presets"/"record.json")) throw std::runtime_error("trash files not removed");
        WritePreset(trashPreset,deletionRoot/"Presets"/"record.json");
        {std::ofstream out(deletionRoot/"Captures"/"trash.png"); out<<"image";}
        std::vector<std::string> trace;
        const DeletionTrace collect=[&](const std::string& line){trace.push_back(line);};
        const auto hasTrace=[&](const std::string& text){for(const auto& line:trace) if(line.find(text)!=std::string::npos) return true; return false;};
        {std::ofstream out(deletionRoot/"Presets"/"unrelated-broken.json"); out<<"{";}
        caught=false;
        try {DeleteTrashedFiles("trash.png",trashLib,deletionRoot,collect);}catch(const std::exception&){caught=true;}
        if(!caught || !hasTrace("read_preset") || !hasTrace("unrelated-broken.json") || !hasTrace("failed") || !std::filesystem::exists(deletionRoot/"Captures"/"trash.png")) throw std::runtime_error("failed read not diagnosed before deletion");
        std::filesystem::remove(deletionRoot/"Presets"/"unrelated-broken.json");
        trace.clear();
        DeleteTrashedFiles("trash.png",trashLib,deletionRoot,collect);
        if(!hasTrace("remove_result") || !hasTrace("actual_parent") || !hasTrace("complete") || !hasTrace("handle_path") || !hasTrace("\"removed\":true")) throw std::runtime_error("deletion trace missing result or path evidence");
        if(std::filesystem::exists(deletionRoot/"Captures"/"trash.png") || std::filesystem::exists(deletionRoot/"Presets"/"record.json") || !std::filesystem::exists(deletionRoot/"Presets"/"keep.json")) throw std::runtime_error("traced deletion changed scope");
        // A broken logging sink must never prevent or broaden normal deletion.
        WritePreset(trashPreset,deletionRoot/"Presets"/"record.json");
        {std::ofstream out(deletionRoot/"Captures"/"trash.png"); out<<"image";}
        DeleteTrashedFiles("trash.png",trashLib,deletionRoot,[](const std::string&){throw std::runtime_error("diagnostic sink failed");});
        if(std::filesystem::exists(deletionRoot/"Captures"/"trash.png")) throw std::runtime_error("logging sink blocked deletion");
        // Non-regular targets still fail, with a specific stage in the trace.
        std::filesystem::create_directory(deletionRoot/"Captures"/"trash.png");
        trace.clear(); caught=false;
        try {DeleteTrashedFiles("trash.png",trashLib,deletionRoot,collect);}catch(const std::exception&){caught=true;}
        if(!caught || !hasTrace("regular_file_check") || !std::filesystem::is_directory(deletionRoot/"Captures"/"trash.png")) throw std::runtime_error("directory deletion guard changed");
        std::filesystem::remove(deletionRoot/"Captures"/"trash.png");
        trashLib.labels["trash.png"].deleted=false;
        trace.clear(); caught=false;
        try {DeleteTrashedFiles("trash.png",trashLib,deletionRoot,collect);}catch(const std::exception&){caught=true;}
        if(!caught || !hasTrace("library") || !hasTrace("failed")) throw std::runtime_error("non-trash trace missing");
        trashLib.labels["../outside.png"].deleted=true; caught=false;
        try {DeleteTrashedFiles("../outside.png",trashLib,deletionRoot);}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("trash traversal accepted");
        if(hold.Update(true,true,0,.8f)||hold.Update(false,true,.3f,.8f)||hold.Update(false,false,.3f,.8f)) throw std::runtime_error("short press fired");
        if(hold.Update(true,true,0,.8f)||!hold.Update(false,true,.9f,.8f)||hold.Update(false,true,1.5f,.8f)||hold.Update(false,false,1.5f,.8f)) throw std::runtime_error("hold did not fire exactly once");
        if(hold.Update(true,true,0,.8f)||!hold.Update(false,true,.9f,.8f)) throw std::runtime_error("second hold failed");
        hold={}; if(hold.Update(false,true,1.f,.8f)) throw std::runtime_error("unarmed hold fired after rebind");
        std::filesystem::remove_all(folder);
        std::cout << "PASS: UTF-8 roundtrip, local IDs, dual-wield entries, traversal/ID/duplicate/hand rejection, broken JSON\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
