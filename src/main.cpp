#include "PointerPositionScope.h"
#include "EquipmentRemoval.h"
#include "AccessoryToggle.h"
#include "ItemIdentity.h"
#include "Transmog.h"
#include "TransmogIdentity.h"
#include <random>
#include <tuple>
#include "RandomTryOnNames.h"
#include "EquipmentPluginCatalog.h"
#include "TryOnCustomization.h"
#include <d3d11.h>
#include <wrl/client.h>
#include "Capture.h"
#include "PhotoUpdate.h"
#include "ExternalTranslation.h"
#include "TranslationKeys.h"
#include "VariationGroups.h"
#include "VariationStripLayout.h"
#include <fstream>
#include "PreviewLayout.h"
#include "GalleryLayout.h"
#include "UiScale.h"
#include "MissingPresetCheck.h"
#include "Presets.h"
#include "GalleryOrganize.h"
#include "PresetEditing.h"
#include "ManagedItems.h"
#include "Diagnostics.h"
#include "FollowerOutfits.h"
#include "FollowerAutoOutfits.h"
#include "FollowerAutoState.h"
#include "AutoOutfitPolicy.h"
#include "CraftSession.h"
#include "GuildLocationRules.h"
#include "SceneSuspendPolicy.h"
#include "ActivePluginLookup.h"
#include "StudioSafety.h"
#include "StudioMovement.h"
#include "HoldButton.h"
#include "HotkeyChord.h"
#pragma warning(push)
#pragma warning(disable: 4099 5054)
#include "SKSEMenuFramework.h"
#include "SmoothCamAPI.h"
#pragma push_macro("GetModuleHandle")
#undef GetModuleHandle
#define GetModuleHandle GetModuleHandleA
#include "TrueDirectionalMovementAPI.h"
#pragma pop_macro("GetModuleHandle")
#pragma warning(pop)

using namespace REL::literals;
SKSEPluginInfo(
    .Version = "2.2.0.0"_v,
    .Name = "OutfitGallery",
    .Author = "Outfit Gallery contributors",
    // SKSE's final component is the store identifier: GOG is 1, not 0.
    // RuntimeVersion() retains it; the load guard below uses this same list.
    .RuntimeCompatibility = { "1.5.97.0"_v, "1.6.353.0"_v, "1.6.640.0"_v, "1.6.1130.0"_v, "1.6.1170.0"_v, "1.6.1179.1"_v, "1.7.104.0"_v, "0.0.0.0"_v }
)

namespace Gallery {
namespace UI = ImGuiMCP;
#include "UiScaleScope.inc"
#include "Localization.inc"
SKSEMenuFramework::Model::WindowInterface* window{};
SKSEMenuFramework::Model::WindowInterface* noticeWindow{};
std::atomic<ULONGLONG> noticeUntil{};
std::atomic<bool> noticeCompact{};
SmoothCamAPI::IVSmoothCam3* smooth{};
TDM_API::IVTDM2* tdm{};
std::atomic<bool> active{}, ready{}, pending{}, captureRequested{};
std::atomic<unsigned> command{}, epoch{};
std::atomic<unsigned> studioOpenSerial{};
std::atomic<bool> upscalerCompatibility{}; // persisted option, applied on the next open
unsigned captureDelay{}; // render-thread only, lets the last camera edit reach the game
const StudioSettings initialCamera{};
std::string startupTab=initialCamera.startupTab;
std::atomic<float> distance{initialCamera.distance}, height{initialCamera.height}, orbit{initialCamera.orbit}, pitch{initialCamera.pitch}, fov{initialCamera.fov}, lateral{initialCamera.lateral}, elevation{initialCamera.elevation};
bool showNames=true, showCounts=false;
bool compactGallery{}, compactOrganizeRequested{};
bool showEquipmentTools{true};
// Manual and automatic player outfits share the per-save appearance mode.
float uiZoom=1.f;
bool addMissing{true}, showCameraSettings{}; std::atomic<bool> preferEnchanted{true}; std::string enchantNotice;
std::atomic<bool> fullOutfitToggle{};
std::atomic<bool> detachedPreview{}, cameraCentered{}, followerTargeting{true};
RE::ActorHandle studioActor, verificationActor; // game-thread only, never raw retained pointers
std::atomic<bool> followerMaintained{};
float targetHeading{}; // follower camera does not chase idle turning
std::string studioActorName; // statusMutex, copied for UI
std::atomic<bool> studioFollower{};
std::atomic<std::uint32_t> studioActorID{};
std::array<float,4> previewRect{.54f,.08f,.45f,.84f};
bool resetPreviewRect=true;
unsigned previewSettled{}; // render-thread only
std::atomic<std::uint32_t> partialSlots{0x1803};
std::optional<Preset> revealAfterSave; // guarded by presetsMutex
bool portraitThumbnails=false, thumbnailModeChanged=false, layoutButtonFocused=false;
float thumbnailVisibleWidth=1.f;
int gridColumns=3; // render thread after initialization
std::atomic<bool> allowFreeCamera{true}, allowCombatGallery{true};
CameraBank cameraBank;
int cameraSlot{};
bool cameraBankWritable=true;
const std::filesystem::path cameraBankFile="Data/SKSE/Plugins/OutfitGallery/CameraPresets.json";
ULONGLONG settingsChangedAt{};
const std::filesystem::path settingsFile="Data/SKSE/Plugins/OutfitGallery/StudioSettings.json";
void SaveSettingsIfDue(bool force=false) {
    if (!force && (!settingsChangedAt || GetTickCount64()-settingsChangedAt<600)) return;
    try { WriteStudioSettings({distance.load(),height.load(),orbit.load(),pitch.load(),fov.load(),gridColumns,showNames,showCounts,language,allowFreeCamera.load(),addMissing,lateral.load(),elevation.load(),partialSlots.load(),startupTab,detachedPreview.load(),previewRect,followerTargeting.load(),preferEnchanted.load(),portraitThumbnails,cleanupCompatibility.load(),allowCombatGallery.load(),compactGallery,reclaimRenamedGeneratedItems.load(),uiZoom,fullOutfitToggle.load(),upscalerCompatibility.load(),showEquipmentTools},settingsFile); }
    catch(const std::exception& e) { SKSE::log::error("Settings save: {}",e.what()); }
    settingsChangedAt=0;
}
std::mutex statusMutex;
std::string status = "Save equipment with F9; click a photo to apply a preset.";
std::string lastImage;
std::atomic<unsigned> keyboardModifiers{};
std::atomic<unsigned> hotkey{66}; // DIK_F8, configurable in the INI.
unsigned saveHotkey = 67; // legacy INI fallback
std::array<std::atomic<unsigned>,3> captureKeys{67,68,87}, capturePads{32768,16384,64};
std::atomic<unsigned> padEdges{}, padHeld{};
// Physical state also tracks the opening press while the gallery is closed.
std::atomic<unsigned> physicalPadHeld{}, openingPadBlock{};
std::atomic<unsigned> padResetSerial{};
// Physical input determines pointer ownership; a recentered stationary cursor is not mouse input.
std::atomic<bool> galleryPadPointer{};
void ResetGalleryPadInput(const char*) {
    padHeld.exchange(0); padEdges.exchange(0);
    ++padResetSerial;
}
bool helpOpen{};
std::atomic<bool> editPending{};
std::atomic<bool> captureAllowed{};
std::atomic<bool> organizerInputActive{};
std::atomic<unsigned> gamepadHotkey{32}; // View / Back; configurable, 0 disables.
std::atomic<float> padHoldSeconds{.8f};
std::atomic<unsigned> bindingMode{}, capturedBinding{};
std::atomic<ULONGLONG> bindingDeadline{};
HoldButton padHold, autoPadHold;
std::atomic<unsigned> autoKeyboard{},autoModifiers{},autoGamepad{};
std::atomic<float> autoHoldSeconds{.8f};
std::atomic<bool> autoToggleRequested{};
void ProcessAutoToggle();
const std::filesystem::path inputSettingsFile="Data/SKSE/Plugins/OutfitGallery/Hotkeys.json";
std::atomic<bool> saveBusy{};
std::atomic<bool> photoRetakeActive{}, photoRetakeCaptureRequested{};
PhotoUpdate photoUpdate{"Data/SKSE/Plugins/OutfitGallery/Captures"}; // render-thread only
std::string photoRetakeName;
unsigned photoRetakeSerial{};
bool photoRetakeReveal{};
unsigned photoRetakeLayoutFrames{}; // render-only; settle scroll limits after a column change
std::mutex presetsMutex;
std::vector<Preset> presets;
Preset stagedPreset, requestedPreset;
// Request/response values only; no game pointers cross to the renderer.
Preset scopeRequest;
std::vector<std::uint32_t> scopeMasks;
std::string scopeError;
bool scopeReady{}; // guarded by presetsMutex

Preset verificationTarget;
std::atomic<unsigned> verificationTicks{}; // also read by the render-thread task scheduler
ULONGLONG verificationDue{}; bool verificationPrefer{};
std::string requestedName;


bool requestedAddMissing{};
const std::filesystem::path presetFolder = "Data/SKSE/Plugins/OutfitGallery/Presets";

std::atomic<unsigned> presetRevision{};
void ReloadPresets() {
    std::vector<Preset> found;
    if (std::filesystem::exists(presetFolder)) {
        for (const auto& f : std::filesystem::directory_iterator(presetFolder)) {
            if (f.path().extension() != ".json") continue;
            try { auto p=ReadPreset(f.path());
                found.push_back(std::move(p)); }
            catch (const std::exception& e) { SKSE::log::warn("Skipped preset {}: {}",f.path().string(),e.what()); }
        }
    }
    std::sort(found.begin(),found.end(),[](const auto& a,const auto& b){return a.photo > b.photo;});
    std::scoped_lock lock(presetsMutex); presets = std::move(found); ++presetRevision;
}
bool ownsSmooth{}, ownsDirection{}, ownsHead{};
struct Snapshot {
    bool valid{}, firstPerson{}, menusVisible{};
    float worldFov{}, thirdZoom{}, thirdTargetZoom{}, thirdSavedZoom{};
    RE::NiPoint2 thirdRotation{};
    RE::NiPoint3 freeTranslation{};
    RE::BSTPoint2<float> freeRotation{};
    bool wasFree{}, hidAllMenus{}, requestedCursorMenu{}, linkedCursorMenu{};
} saved;

struct HudVisibilitySnapshot {
    RE::GPtr<RE::HUDMenu> menu;
    RE::GFxValue root;
    bool visible;
};
std::optional<HudVisibilitySnapshot> savedHud; // game-thread only

bool HideStudioHud(RE::UI& ui) {
    auto menu = ui.GetMenu<RE::HUDMenu>();
    if (!menu || !menu->uiMovie) return false;

    const auto& root = menu->GetRuntimeData().root;
    RE::GFxValue::DisplayInfo previous;
    if (!root.IsDisplayObject() || !root.GetDisplayInfo(&previous)) return false;

    // Hold the specific HUD movie/object until Close; leave the game menu flag alone.
    savedHud.emplace(HudVisibilitySnapshot{std::move(menu), root, previous.GetVisible()});
    if (!savedHud->visible) return true;

    RE::GFxValue::DisplayInfo hidden;
    hidden.SetVisible(false);
    if (savedHud->root.SetDisplayInfo(hidden)) return true;
    savedHud.reset();
    return false;
}

void RestoreStudioHud() {
    if (!savedHud) return;
    if (savedHud->visible) {
        RE::GFxValue::DisplayInfo visible;
        visible.SetVisible(true);
        if (!savedHud->root.SetDisplayInfo(visible))
            SKSE::log::warn("HUD1: could not restore the original HUD movie");
    }
    savedHud.reset();
}

struct CursorVisibilitySnapshot {
    RE::GPtr<RE::GFxMovieView> movie;
    bool visible;
};
std::optional<CursorVisibilitySnapshot> savedCursor; // game-thread only

void RestoreStudioCursor() {
    if (!savedCursor) return;
    savedCursor->movie->SetVisible(savedCursor->visible);
    savedCursor.reset();
}

void HideStudioCursor() {
    if (!saved.linkedCursorMenu) return;
    auto* ui = RE::UI::GetSingleton();
    auto menu = ui ? ui->GetMenu<RE::CursorMenu>() : nullptr;
    if (!menu || !menu->uiMovie) return;

    // The menu remains open for HUD listeners; only its Flash movie is hidden.
    if (!savedCursor || savedCursor->movie.get() != menu->uiMovie.get()) {
        RestoreStudioCursor();
        savedCursor.emplace(CursorVisibilitySnapshot{menu->uiMovie, menu->uiMovie->GetVisible()});
        SKSE::log::info("HUD3 Cursor Menu movie captured; originallyVisible={}", savedCursor->visible);
    }
    menu->uiMovie->SetVisible(false);
}

void SetStatus(std::string text) {
    SKSE::log::info("{}", text);
    std::scoped_lock lock(statusMutex);
    status = std::move(text);
}

// A non-blocking framework notice also works when the studio cannot open.
void Refuse(std::string reason) {
    noticeCompact=false;
    SetStatus("Cannot open studio: " + reason);
    noticeUntil = GetTickCount64() + 8000;
    if (noticeWindow) noticeWindow->IsOpen = true;
}

void __stdcall RenderNotice() {
    if (GetTickCount64() >= noticeUntil) { noticeWindow->IsOpen = false; return; }
    UI::SetNextWindowPos({30.f, 60.f}, UI::ImGuiCond_Always);
    UI::SetNextWindowSize({620.f, 0.f}, UI::ImGuiCond_Always);
    if (UI::Begin("Outfit Gallery - Status", nullptr, UI::ImGuiWindowFlags_NoInputs | UI::ImGuiWindowFlags_NoCollapse | UI::ImGuiWindowFlags_NoResize)) {
        std::scoped_lock lock(statusMutex);
        UI::TextWrapped(Tr("%s"), StatusText(status).c_str());
        if(!noticeCompact) UI::TextUnformatted(Tr("Details: Documents/My Games/Skyrim Special Edition/SKSE/OutfitGallery.log"));
    }
    UI::End();
}

void Release() {
    const auto handle = SKSE::GetPluginHandle();
    if (ownsHead && tdm) { (void)tdm->ReleaseDisableHeadtracking(handle); ownsHead = false; }
    if (ownsDirection && tdm) { (void)tdm->ReleaseDisableDirectionalMovement(handle); ownsDirection = false; }
    if (ownsSmooth && smooth) { (void)smooth->ReleaseCameraControl(handle); ownsSmooth = false; }
}

// Game-thread only. Also called before a save is loaded so no previous-session
// camera snapshot is ever applied to the newly loaded game.
void Close() {
    openingPadBlock=0;
    ResetGalleryPadInput("close/preload");
    editPending=false;
    bindingMode=0;
    active = false;
    captureRequested = false;
    photoRetakeCaptureRequested = false;
    saveBusy = false;
    if (window) window->IsOpen = false;
    if (saved.valid) {
        auto* camera = RE::PlayerCamera::GetSingleton();
        if (camera) {
            if (!saved.wasFree && camera->IsInFreeCameraMode()) camera->ToggleFreeCameraMode(false);
            // Select the original normal view before restoring its numeric state.
            if (!saved.wasFree && (camera->IsInFirstPerson() || camera->IsInThirdPerson())) {
                if (saved.firstPerson) camera->ForceFirstPerson();
                else camera->ForceThirdPerson();
            }
            camera->GetRuntimeData2().worldFOV = saved.worldFov;
            auto* third = static_cast<RE::ThirdPersonState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kThirdPerson].get());
            if (third) {
                third->currentZoomOffset = saved.thirdZoom;
                third->targetZoomOffset = saved.thirdTargetZoom;
                third->savedZoomOffset = saved.thirdSavedZoom;
                third->freeRotation = saved.thirdRotation;
            }
            auto* free = static_cast<RE::FreeCameraState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kFree].get());
            if (free) { free->translation = saved.freeTranslation; free->rotation = saved.freeRotation; }
        }
        RestoreStudioHud();
        RestoreStudioCursor();
        if (saved.requestedCursorMenu) {
            if (auto* queue = RE::UIMessageQueue::GetSingleton()) {
                // Pair the queued show even if the engine has yet to process it.
                queue->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
                SKSE::log::info("HUD3 Cursor Menu hide queued");
            }
        }
        if (saved.hidAllMenus) {
            if (auto* ui = RE::UI::GetSingleton()) ui->ShowMenus(saved.menusVisible);
        }
        saved.valid = false;
    }
    EndTransmogTryOn(RE::PlayerCharacter::GetSingleton());
    studioActor.reset(); studioFollower=false; studioActorID=0;
    Release();
}

bool AcquireStudioHeadtracking() {
    TDM_API::APIResult result{};
    const auto state = RequestStudioHeadtracking(*tdm, SKSE::GetPluginHandle(),
        static_cast<SKSE::PluginHandle>(-1), result);
    if (!active || state != StudioHeadtracking::External)
        SKSE::log::info("OG-HEADTRACK control={} result={} owner={}", static_cast<unsigned>(state),
            static_cast<unsigned>(result), tdm->GetDisableHeadtrackingOwner());
    if (state == StudioHeadtracking::Denied) {
        SetStatus("TDM did not grant head tracking control.");
        return false;
    }
    ownsHead = state == StudioHeadtracking::Owned;
    return true;
}

bool Acquire() {
    const auto handle = SKSE::GetPluginHandle();
    if (GetModuleHandleW(L"SmoothCam.dll")) {
        if (!smooth) { SetStatus("SmoothCam is installed but its API is unavailable."); return false; }
        auto result = smooth->RequestCameraControl(handle);
        if (result != SmoothCamAPI::APIResult::OK && result != SmoothCamAPI::APIResult::AlreadyGiven) {
            SetStatus(std::format("SmoothCam did not grant control ({})", static_cast<int>(result))); return false;
        }
        ownsSmooth = true;
    }
    if (GetModuleHandleW(L"TrueDirectionalMovement.dll")) {
        if (!tdm) { Release(); SetStatus("TDM is installed but its API is unavailable."); return false; }
        if (tdm->GetTargetLockState()) { Release(); SetStatus("Exit target lock before opening the studio."); return false; }
        TDM_API::APIResult result{};
        const bool fluidLoaded = GetModuleHandleW(L"FluidMotionAnimation.dll") != nullptr;
        const auto movement = RequestStudioMovement(*tdm, handle, static_cast<SKSE::PluginHandle>(-1), fluidLoaded, result);
        SKSE::log::info("OG-FLUID-TEST1 movement result={} fluidLoaded={} owner={} mode={}",
            static_cast<unsigned>(result), fluidLoaded, tdm->GetDisableDirectionalMovementOwner(), static_cast<unsigned>(movement));
        if (movement == StudioMovement::Denied) {
            Release(); SetStatus("TDM did not grant movement control."); return false;
        }
        ownsDirection = movement == StudioMovement::Owned;
        if (movement == StudioMovement::External)
            SKSE::log::info("OG-FLUID-TEST1 external movement owner retained; entering free camera using actor heading. No external release.");
        if (!AcquireStudioHeadtracking()) {
            Release(); return false;
        }
    }
    return true;
}

bool StudioActorUnsafe(RE::Actor* actor) {
    if(!actor || !actor->Get3D() || actor->IsDisabled() || actor->IsDead()) return true;
    const auto* state=actor->AsActorState();
    return actor->IsInKillMove() || actor->IsInRagdollState() || state->IsUnconscious() || state->IsStaggered() ||
        actor->IsOnMount() || state->IsSwimming() || state->GetSitSleepState()!=RE::SIT_SLEEP_STATE::kNormal;
}
void Open() {
    if (!ready || active || !window) return;
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* camera = RE::PlayerCamera::GetSingleton();
    auto* ui = RE::UI::GetSingleton();
    if (!player || !player->Get3D() || !camera || !ui) { SetStatus("Load a game before opening the studio."); return; }
    const bool paused = ui->GameIsPaused(), dead = player->IsDead(), combat = player->IsInCombat();
    // ActorState moved from 0xB8 to 0xC0 in AE 1.6.629+. An inherited
    // ActorState call uses the compile-time base offset in this multi-runtime
    // build; use CommonLib's version-aware base accessor instead.
    const auto* actorState = player->AsActorState();
    const bool mounted = player->IsOnMount(), swimming = actorState->IsSwimming();
    const auto sitting = actorState->GetSitSleepState();
    SKSE::log::info("ActorState runtime offset: {:#x}", reinterpret_cast<std::uintptr_t>(actorState) - reinterpret_cast<std::uintptr_t>(player));
    SKSE::log::info("Open request: paused={}, pauseCount={}, dead={}, combat={}, mounted={}, swimming={}, sitSleep={}, firstPerson={}, thirdPerson={}",
        paused, ui->numPausesGame, dead, combat, mounted, swimming, static_cast<unsigned>(sitting), camera->IsInFirstPerson(), camera->IsInThirdPerson());
    if (paused || dead || !StudioCombatAllowed(combat,combat,allowCombatGallery.load()) || mounted || swimming || sitting != RE::SIT_SLEEP_STATE::kNormal) {
        std::string reason;
        if (paused) reason += "Game paused. ";
        if (dead) reason += "Player dead. ";
        if (combat && !allowCombatGallery.load()) reason += "Player in combat. ";
        if (mounted) reason += "Player mounted. ";
        if (swimming) reason += "Player swimming. ";
        if (sitting != RE::SIT_SLEEP_STATE::kNormal) reason += std::format("Sit/sleep state {}. ", static_cast<unsigned>(sitting));
        Refuse(reason); return;
    }
    if(StudioActorUnsafe(player)) {Refuse("Player is in an unsafe animation or unavailable.");return;}
    RE::NiPointer<RE::Actor> target{player};
    if(followerTargeting) {
        auto* pick=RE::CrosshairPickData::GetSingleton();
        auto reference=pick?pick->GetActiveTarget().get():RE::NiPointer<RE::TESObjectREFR>{};
        if(reference) if(auto* actor=reference->As<RE::Actor>(); actor && actor!=player) {
            if(!IsCurrentOutfitFollower(actor) || !actor->HasKeywordString("ActorTypeNPC")) {
                Refuse("Aim at a humanoid current follower, or look away to select yourself."); return;
            }
            const auto* reason=FollowerUnsafeReason(actor);
            SKSE::log::info("OG-FOLLOWER state actor={:08X} sitSleep={} scene={} stagger={} combat={} reason={}",
                actor->GetFormID(),static_cast<unsigned>(actor->AsActorState()->GetSitSleepState()),
                actor->GetCurrentScene()!=nullptr,actor->AsActorState()->IsStaggered(),actor->IsInCombat(),reason?reason:"none");
            if(reason) {Refuse(reason); return;}
            if(!StudioCombatAllowed(combat,actor->IsInCombat(),allowCombatGallery.load())) {
                Refuse("Player or follower is in combat."); return;
            }
            target=RE::NiPointer<RE::Actor>{actor};
        }
    }
    SKSE::log::info("OG-STUDIO combatEntry={} playerCombat={} playerTarget={}",allowCombatGallery.load(),combat,target.get()==player);
    const bool wasFree=camera->IsInFreeCameraMode();
    if (!camera->IsInFirstPerson() && !camera->IsInThirdPerson() && !(wasFree && allowFreeCamera.load())) { SetStatus("Use a normal first/third-person view, or enable free-camera entry."); return; }
    auto* third = static_cast<RE::ThirdPersonState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kThirdPerson].get());
    auto* free = static_cast<RE::FreeCameraState*>(camera->GetRuntimeData().cameraStates[RE::CameraState::kFree].get());
    if (!third || !free || !Acquire()) return;
    saved = {true, camera->IsInFirstPerson(), ui->IsShowingMenus(), camera->GetRuntimeData2().worldFOV,
        third->currentZoomOffset, third->targetZoomOffset, third->savedZoomOffset, third->freeRotation, free->translation, free->rotation};
    saved.wasFree=wasFree;
    if(!wasFree) {camera->ForceThirdPerson(); camera->ToggleFreeCameraMode(false);}
    if (!camera->IsInFreeCameraMode()) { Close(); SetStatus("Could not enter the studio camera."); return; }
    studioActor=target->CreateRefHandle(); targetHeading=target->GetAngleZ();
    studioFollower=target.get()!=player; studioActorID=target->GetFormID();
    followerMaintained=IsFollowerOutfitMaintained(target.get());
    {std::scoped_lock lock(statusMutex); studioActorName=target->GetName()?target->GetName():"";}
    SKSE::log::info("Studio target: {} ({:08X}), follower={}",target->GetName(),target->GetFormID(),studioFollower.load());
    if (upscalerCompatibility.load()) {
        const bool hidHud = HideStudioHud(*ui);
        const bool cursorWasOpen = ui->IsMenuOpen(RE::CursorMenu::MENU_NAME);
        if (!cursorWasOpen) {
            if (auto* queue = RE::UIMessageQueue::GetSingleton()) {
                queue->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
                saved.requestedCursorMenu = true;
            }
        }
        saved.linkedCursorMenu = cursorWasOpen || saved.requestedCursorMenu;
        HideStudioCursor();
        SKSE::log::info("HUD3 Cursor Menu alreadyOpen={} showQueued={}", cursorWasOpen, saved.requestedCursorMenu);
        SKSE::log::info("HUD1 mode=hud-only hudAvailable={} gameMenusVisible={}", hidHud, ui->IsShowingMenus());
    } else {
        ui->ShowMenus(false);
        saved.hidAllMenus = true;
        SKSE::log::info("HUD1 mode=all-menus");
    }
    active = true;
    ResetGalleryPadInput("open");
    ++studioOpenSerial;
    window->IsOpen = true;
    SetStatus("Adjust composition and press F9/Y to save.");
}

#include "OStimCompatibility.inc"
ULONGLONG equipmentLaneDue{}; // game-thread shared apply spacing
bool FollowerAutoVerificationDone(bool);
void ResetFollowerAutoRuntime();
void FollowerAutoManualChange(RE::Actor*);
bool FollowerAutoVerificationBlocked(RE::Actor*);

std::vector<std::uint32_t> verificationRemovalIDs;
bool verificationRemoving{}; // manual accessory removal
bool autoVerifying{}; // game thread only
void AutoVerificationDone(bool ok);
void VerifyEquipment(bool immediate=false) {
    if(!verificationTicks) return;
    if((autoVerifying && OStimBlocksAutomatic(RE::PlayerCharacter::GetSingleton())) || FollowerAutoVerificationBlocked(verificationActor.get().get())) {
        verificationTicks=0; AutoVerificationDone(false);
        SKSE::log::info("OG-AUTO OStim: pending automatic verification/cleanup cancelled");
        return;
    }
    const bool due=GetTickCount64()>=verificationDue;
    if(!immediate && !due) return;
    try {
        auto actor=verificationActor.get();
        if(!actor || !actor->Get3D() || actor->IsDisabled() || actor->IsDead()) {verificationTicks=0; AutoVerificationDone(false); return;}
        if(verificationRemoving) {
            if(verificationRemovalIDs.empty()?AccessoriesRemoved(actor.get(),verificationTarget):EquipmentIDsRemoved(actor.get(),verificationRemovalIDs)) {
                verificationTicks=0;verificationRemoving=false;
                RefreshFollowerAfterAccessoryRemoval(actor.get());
                SetStatus("Equipment set removed.");
                AutoVerificationDone(true);
            } else if(due) {
                verificationTicks=0;verificationRemoving=false;
                SetStatus("Some equipment could not be removed.");
                AutoVerificationDone(false);
            }
            return;
        }
        const auto actual=SnapshotForVerification(actor.get(),verificationTarget);
        if(SameEquipment(verificationTarget,actual)) {
            verificationTicks=0;
            GALLERY_DIAG("OG-DIAG verify matched preset={} actor={:08X}; cleanup allowed",verificationTarget.name,actor->GetFormID());
            ReclaimManagedItems(actor.get());
            RefreshFollowerOutfit(actor.get());
            if(verificationPrefer) enchantNotice=VerifyPreferredEquipment(actor.get(),verificationTarget);
            SetStatus("Outfit restored: "+verificationTarget.name+enchantNotice);
            AutoVerificationDone(true);
        } else if(due) {
            verificationTicks=0;
            GALLERY_DIAG("OG-DIAG verify mismatch preset={}; cleanup skipped",verificationTarget.name);
            SetStatus("Equipment differs after apply: "+verificationTarget.name);
            AutoVerificationDone(false);
        }
    }catch(const std::exception& e) {
        if(due) {verificationTicks=0; AutoVerificationDone(false); SetStatus(std::string("Equipment verification: ")+e.what());}
    }
}
#include "PresetEditRuntime.inc"
#include "AutoOutfits.inc"
#include "CraftOutfits.inc"
#include "FollowerAutoOutfits.inc"
bool TargetAppearanceMode(std::uint32_t id){return id && id!=0x14?FollowerTransmogMode(id):autoAppearanceOnly.load();}
bool PlayerTransmogMode(){return autoAppearanceOnly.load();}
bool PlayerTransmogAutoOwned(){return autoAppearanceOwned.load();}
void RestorePlayerTransmogMode(bool mode,bool owned){autoAppearanceOnly=mode;autoAppearanceOwned=owned;}
#include "EquipmentRemovalRuntime.inc"
#include "RandomTryOn.inc"
void Tick() {
    const unsigned requested = command.exchange(0);
    if (requested && requested != kRandomTryOn && requested != kRerollTryOn && requested != kOpenTryOnCustom && requested != kChangeTryOnCustom &&
        requested != kPrepareTryOnItems && requested != kAcquireTryOnItems && requested != 2 && requested != 3 && requested != 5 && requested != 6) {
        if (EndTransmogTryOn(RE::PlayerCharacter::GetSingleton()))
            SetStatus(language == 1 ? "試着前の外見に戻しました。" : "Restored the appearance from before try-on.");
    }
    TickTransmogDiagnostic();
    for(auto id:TransmogAutoOwnedActors()) {
        auto* actor=RE::TESForm::LookupByID<RE::Actor>(id);
        if(actor && (!FollowerAutomaticOwns(id) || !IsCurrentOutfitFollower(actor))) ClearActorTransmog(actor);
    }
    if((requested==32 || requested==33) && !verificationTicks && !saveBusy && !craftActive) {
        const bool enabled=requested==32;
        if(enabled && !TransmogAvailable()) SetStatus(TransmogUnavailableReason(language==1));
        if(studioFollower) {
            auto actor=studioActor.get();
            if(actor && IsCurrentOutfitFollower(actor.get()) && (!enabled || TransmogAvailable())) {
                if(!enabled) {
                    const auto owned=TransmogAutoOwnedActors();
                    if(std::find(owned.begin(),owned.end(),actor->GetFormID())!=owned.end()) ClearActorTransmog(actor.get());
                }
                SetFollowerTransmogMode(actor.get(),enabled);
                std::scoped_lock lock(followerAutoMutex);
                if(auto it=followerAutos.find(actor->GetFormID());it!=followerAutos.end()) {
                    it->second.policy={};it->second.style={};it->second.combat={};it->second.arrival.Cancel();++it->second.revision;followerAutoDue=0;
                }
            }
        } else if(enabled!=autoAppearanceOnly.load() && (!enabled || TransmogAvailable())) {
            if(!enabled && autoAppearanceOwned) ClearTransmog();
            autoAppearanceOwned=false;autoAppearanceOnly=enabled;
            autoPolicy={};autoStyle={};autoCombat={};++autoRevision;autoDue=0;
            SKSE::log::info("OG-AUTO-TRANSMOG enabled={}; stored on next game save",enabled);
        }
    }
    if(requested==31) {
        auto actor=studioActor.get();
        if(actor) {ClearActorTransmog(actor.get());AutoManualChange(actor.get());}
        if(!studioFollower) autoAppearanceOwned=false;
        SetStatus("Appearance restored to actual equipment.");
    }
    if(requested==1 || requested==2) GALLERY_DIAG("OG-OPEN-DIAG tick command={} active={}",requested,active.load());
    // Pending ownership cleanup outlives the camera window, but is cancelled
    // explicitly at save/new-game boundaries before touching another character.
    VerifyEquipment(requested==2 || requested==4);
    if (requested == 2) { Close(); return; }
    if (requested == 1) Open();
    if (!active) {
        if(!verificationTicks && GetTickCount64()>=equipmentLaneDue) MaintainFollowerOutfits();
        // Poll the player before every follower pass, even if their one-second
        // polling clocks differ. Ready player work claims the lane first;
        // an unsafe or not-yet-ready player never holds followers back.
        MaintainAutoOutfits(FollowerAutoDue()); MaintainFollowerAutoOutfits(); return;
    }
    HideStudioCursor(); // The queued Cursor Menu show may complete after Open().
    // Another mod may release its disable grant after Gallery switches cameras.
    if (tdm && !ownsHead && !AcquireStudioHeadtracking()) { Close(); return; }
    MaintainFollowerAutoOutfits();
    if(AutoDue()) MaintainAutoOutfits();
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!window->IsOpen || StudioActorUnsafe(player) || !camera || !camera->IsInFreeCameraMode() || (tdm && tdm->GetTargetLockState())) { GALLERY_DIAG("OG-OPEN-DIAG state-close window={} unsafe={} camera={} free={} lock={}",window->IsOpen.load(),StudioActorUnsafe(player),camera!=nullptr,camera && camera->IsInFreeCameraMode(),tdm && tdm->GetTargetLockState()); Close(); return; }
    auto subject=studioActor.get();
    const bool follower=subject.get()!=player;
    const char* targetReason=follower?FollowerUnsafeReason(subject.get()):nullptr;
    if(!targetReason && follower && !IsCurrentOutfitFollower(subject.get())) targetReason="Follower is no longer accompanying you.";
    if(!targetReason && subject && !StudioCombatAllowed(player->IsInCombat(),subject->IsInCombat(),allowCombatGallery.load()))
        targetReason="Player or follower is in combat.";
    if(targetReason) {
        SKSE::log::info("OG-FOLLOWER studio closed: {}",targetReason);
        Close(); SetStatus(targetReason); return;
    }
    if (!follower && randomPluginListRequested.exchange(false)) RefreshRandomTryOnPlugins(player);
    if ((requested == kRandomTryOn || requested == kRerollTryOn) && !follower && !saveBusy && !verificationTicks && !photoRetakeActive)
        RunRandomTryOn(player, requested == kRerollTryOn);
    if (!follower) {
        ProcessTryOnCustomization(requested, player);
        ProcessTryOnInventory(requested, player);
    }
    if(requested==30 && !saveBusy) {
        try {
            Preset p; {std::scoped_lock lock(presetsMutex); p=requestedPreset;}
            const bool allowRemoval=AccessoryToggle::Allowed(p.accessories,p.slotMask,fullOutfitToggle.load());
            // Honor the same saved equipment-manager selection as normal equip.
            // Filter before diagnostics/staging so unchecked photo props never
            // enter model validation or replace the current appearance.
            if(p.outfitItems && !p.slotMask && !p.accessories) {
                p=FilterExchangeSlots(p,{});
                if(p.items.empty()) {SetStatus("No equipment selected. Equipment unchanged."); return;}
            } else if(p.exchangeSlots) {
                if(!*p.exchangeSlots && p.exchangeSlotless.empty()) {SetStatus("No exchange slots selected. Equipment unchanged."); return;}
                p=FilterExchangeSlots(p,ResolvePresetSlots(p));
            }
            if(allowRemoval && RemoveEquippedAccessoryAppearance(p,subject.get())) {
                SetTransmogAutoOwned(subject.get(),false);
                if(!follower) autoAppearanceOwned=false;
                AutoManualChange(subject.get());
                SetStatus("Equipment appearance removed. Actual equipment is unchanged.");
                return;
            }
            if(!follower) BeginTransmogDiagnostic(p,true);
            ApplyTransmog(p,subject.get());SetTransmogAutoOwned(subject.get(),false);
            if(!follower) autoAppearanceOwned=false;
            AutoManualChange(subject.get());
            SetStatus("Appearance applied. Actual equipment is unchanged.");
        } catch(const std::exception& e) {SetStatus(e.what());}
    }
    if (!photoRetakeActive && (requested == 3 || requested == 5 || requested == 6)) {
        try {
            auto trial = requested == 3 ? GetTransmogTryOnPreset(subject.get()) : std::nullopt;
            if(!trial && TransmogActive(subject->GetFormID())) throw std::runtime_error("Restore actual appearance before registering equipment.");
            const auto scope=requested==5?partialSlots.load():0;
            if(requested==5 && !scope) throw std::runtime_error("Select at least one slot.");
            auto p = trial ? std::move(*trial) :
                (requested==6?SnapshotAccessories(subject.get(),true,true):SnapshotEquipment(subject.get(),scope,true,true));
            {
                std::scoped_lock lock(presetsMutex);
                if(!requestedName.empty()) p.name = requestedName;
                else if(!trial) {
                    // Match try-on registration: use an item name, preferring
                    // the body piece for full outfits over inventory iteration order.
                    const Item* named=nullptr;
                    for(const auto& item:p.items) {
                        if(item.name.empty()) continue;
                        if(!named) named=&item;
                        if(requested!=3) continue;
                        auto* form=ResolveItem(item);
                        const auto* armor=form?form->As<RE::TESObjectARMO>():nullptr;
                        if(armor && (armor->GetSlotMask().underlying() & (1u<<2))) {named=&item;break;}
                    }
                    p.name=named?named->name:"Outfit "+std::to_string(presets.size()+1);
                }
                stagedPreset = std::move(p);
            }
            captureRequested = true;
        } catch (const std::exception& e) { saveBusy = false; SetStatus(e.what()); }
    }
    ProcessEquipmentRemoval(requested,subject.get());
    ProcessPresetEdit(requested,subject.get());
    if(requested==10) {
        Preset p; {std::scoped_lock lock(presetsMutex); p=scopeRequest;}
        std::vector<std::uint32_t> masks; std::string error;
        try {masks=ResolvePresetSlots(p);} catch(const std::exception& e) {error=e.what();}
        {std::scoped_lock lock(presetsMutex); if(scopeRequest.photo==p.photo) {scopeMasks=std::move(masks); scopeError=std::move(error); scopeReady=true;}}
    }
    if (requested == 4 && !saveBusy) {
        verificationTicks=0;
        try {
            Preset p; bool add;
            { std::scoped_lock lock(presetsMutex); p = requestedPreset; add = requestedAddMissing; }
            const bool allowRemoval=AccessoryToggle::Allowed(p.accessories,p.slotMask,fullOutfitToggle.load());
            if(p.outfitItems && !p.slotMask && !p.accessories) {
                p=FilterExchangeSlots(p,{});
                if(p.items.empty()) {SetStatus("No equipment selected. Equipment unchanged."); return;}
            } else if(p.exchangeSlots) {
                if(!*p.exchangeSlots && p.exchangeSlotless.empty()) {SetStatus("No exchange slots selected. Equipment unchanged."); return;}
                p=FilterExchangeSlots(p,ResolvePresetSlots(p));
            }
            ClearActorTransmog(subject.get());
            AutoManualChange(subject.get());
            if(allowRemoval && RemoveEquippedAccessories(subject.get(),p)) {
                enchantNotice.clear();verificationPrefer=false;autoVerifying=false;
                verificationActor=subject->CreateRefHandle();verificationTarget=p;verificationRemovalIDs.clear();verificationRemoving=true;
                verificationDue=GetTickCount64()+750;verificationTicks=1;
                SetStatus("Equipment removal requested.");
                VerifyEquipment(true);
                return;
            }
            enchantNotice.clear(); verificationPrefer=preferEnchanted.load(); auto result=ApplyEquipment(subject.get(),p,add,verificationPrefer); const auto warning=result.find(" Registered enchantment not restored:"); if(warning!=std::string::npos) enchantNotice=result.substr(warning); SetStatus(result); verificationActor=subject->CreateRefHandle(); verificationRemoving=false;verificationTarget=ClothingPreset(p); verificationDue=GetTickCount64()+750; verificationTicks=1;
            VerifyEquipment(true);
        } catch (const std::exception& e) { SetStatus(e.what()); }
    }
    if(studioFollower && !saveBusy && (requested==7 || requested==8 || requested==9)) {
        try {
            if(requested==7) {
                if(FollowerAutomaticOwns(subject->GetFormID())) throw std::runtime_error("Disable automatic outfits before fixed maintenance.");
                RememberFollowerOutfit(subject.get());
                SetStatus("Follower outfit maintenance enabled.");
            } else {
                ReleaseFollowerOutfit(subject.get());
                if(requested==8) SetStatus("Follower outfit maintenance disabled.");
                else {
                    verificationTicks=0;
                    AutoManualChange(subject.get());
                    ClearActorTransmog(subject.get());
                    const auto original=DefaultFollowerOutfit(subject.get());
                    SetStatus(ApplyEquipment(subject.get(),original,true));
                    verificationActor=subject->CreateRefHandle(); verificationRemoving=false;verificationTarget=original;
                    verificationDue=GetTickCount64()+750; verificationTicks=1;
                    VerifyEquipment(true);
                }
            }
        }catch(const std::exception& e) {SetStatus(e.what());}
    }
    followerMaintained=IsFollowerOutfitMaintained(subject.get());
    auto* free = static_cast<RE::FreeCameraState*>(camera->currentState.get());
    const float radians = std::numbers::pi_v<float> / 180.f;
    const float radial = (studioFollower ? targetHeading : subject->GetAngleZ()) + orbit.load() * radians;
    const float yaw = radial + std::numbers::pi_v<float>;
    const float dist = distance.load();
    const float angle=elevation.load()*radians;
    const float planar=dist*std::cos(angle);
    const bool centered=detachedPreview.load();
    const float side = dist * std::tan(fov.load() * radians * 0.5f) * (centered ? 0.f : 0.53f) + lateral.load();
    const auto pos = subject->GetPosition();
    free->translation = {pos.x + std::sin(radial)*planar - std::cos(yaw)*side,
        pos.y + std::cos(radial)*planar + std::sin(yaw)*side, pos.z + height.load() + dist*std::sin(angle)};
    free->rotation = {pitch.load() * radians + angle, yaw};
    camera->GetRuntimeData2().worldFOV = fov.load();
    camera->rotationInput = {};
    camera->translationInput = {};
    camera->zoomInput = 0;
    cameraCentered=centered;
}

void QueueTick() {
    if ((active || command.load() || verificationTicks.load() || TransmogMaintenanceDue() || FollowerMaintenanceDue() || AutoDue() || FollowerAutoDue()) && !pending.exchange(true)) {
        const unsigned generation = epoch.load();
        SKSE::GetTaskInterface()->AddTask([generation] {
            if (generation == epoch && ready) Tick();
            pending = false;
        });
    }
}

void CancelPhotoRetake() {
    if(!photoUpdate.Draft().empty()) DisposePortraitView(photoUpdate.Draft().generic_string());
    photoUpdate.Cancel();
    photoRetakeActive=false; photoRetakeCaptureRequested=false; photoRetakeReveal=false; photoRetakeLayoutFrames=0;
    captureDelay=0; captureRequested=false; saveBusy=false;
}
void __stdcall OnFrame(SKSEMenuFramework::Model::EventType event) {
    if (event != SKSEMenuFramework::Model::kBeforeRender || !ready) return;
    if(photoRetakeActive && (!active || photoRetakeSerial!=studioOpenSerial.load() || !window || !window->IsOpen)) CancelPhotoRetake();
    ProcessAutoToggle();
    SaveSettingsIfDue();
    // The framework dispatches listeners under its own lock. Never call back into
    // its registration/lookup APIs here. Game mutations go to one coalesced task.
    QueueTick();
    if (active && captureRequested.exchange(false)) captureDelay = 3;
    if (active && photoRetakeActive && photoRetakeCaptureRequested.exchange(false)) captureDelay = 3;
    if (!active) {captureDelay=0; previewSettled=0; ResetLivePreview();}
    else if(cameraCentered.load()!=detachedPreview.load()) {previewSettled=0; ResetLivePreview();}
    else previewSettled=std::min(previewSettled+1,4u);
    if(active && detachedPreview && previewSettled>=3) {
        try {UpdateLivePreview();}
        catch(const std::exception& e) {
            ResetLivePreview(); detachedPreview=false; previewSettled=0;
            captureDelay=0; saveBusy=false; settingsChangedAt=GetTickCount64();
            SetStatus(std::string("Preview disabled: ")+e.what());
        }
    } else if(!detachedPreview) ResetLivePreview();
    if (captureDelay && previewSettled>=3 && --captureDelay == 0 && active) {
        try {
            const auto file = CapturePortrait(detachedPreview.load(),photoRetakeActive.load());
            if(photoRetakeActive) {
                try {
                    if(!LoadPortraitView(file.generic_string())) throw std::runtime_error("Could not preview the new photo. Original photo kept.");
                    if(!photoUpdate.Draft().empty()) DisposePortraitView(photoUpdate.Draft().generic_string());
                    photoUpdate.Stage(file);
                    photoRetakeReveal=true;
                    SetStatus("Photo preview only. Not saved yet.");
                } catch(...) {DisposePortraitView(file.generic_string()); std::error_code ec; std::filesystem::remove(file,ec); throw;}
            } else {
            { std::scoped_lock lock(statusMutex); lastImage = file.generic_string(); }
            Preset p;
            { std::scoped_lock lock(presetsMutex); p = stagedPreset; }
            p.photo = file.filename().string();
            WritePreset(p,presetFolder / (file.stem().string() + ".json"));
            ReloadPresets();
            {std::scoped_lock lock(presetsMutex); revealAfterSave=p;}
            SetStatus("Photo + outfit saved: " + p.name);
            }
        } catch (const std::exception& e) { SetStatus(e.what()); }
        saveBusy = false;
    }
}

void Slider(const char* name, std::atomic<float>& value, float min, float max) {
    float local = value.load();
    if (UI::SliderFloat(Label(name), &local, min, max)) { value = local; settingsChangedAt=GetTickCount64(); }
}

#include "HotkeyUI.inc"
void SaveCameraSlot() {
    if(cameraSlot==0) return; // Slot 1 is always the built-in rescue composition.
    if(!cameraBankWritable) {SetStatus("Camera presets could not be read; writing is disabled."); return;}
    auto next=cameraBank;
    next[cameraSlot]=StudioSettings{distance.load(),height.load(),orbit.load(),pitch.load(),fov.load()};
    next[cameraSlot]->lateral=lateral.load(); next[cameraSlot]->elevation=elevation.load();
    try {WriteCameraBank(next,cameraBankFile); cameraBank=std::move(next); SetStatus("Camera preset saved: "+std::to_string(cameraSlot+1));}
    catch(const std::exception& e){SetStatus(e.what());}
}
void RenderCameraSettings() {
    Slider("Distance",distance,10,400); Slider("Horizontal position",lateral,-150,150); Slider("Vertical position",height,-20,180);
    Slider("Orbit",orbit,-180,180); Slider("Look up / down",elevation,-60,60); Slider("Field of view",fov,35,90);
    if(UI::CollapsingHeader(Label("Fine angle adjustment"))) Slider("Pitch",pitch,-25,25);
    UI::TextUnformatted(Tr("Camera and display settings save automatically."));
    int shownSlot=cameraSlot+1;
    if(UI::SliderInt(Label("Camera preset slot"),&shownSlot,1,7)) cameraSlot=shownSlot-1;
    static int nameSlot=-1;
    static char cameraName[256]{};
    if(nameSlot!=cameraSlot) {
        nameSlot=cameraSlot;
        const auto& name=cameraBank.names[cameraSlot];
        std::copy(name.begin(),name.end(),cameraName);cameraName[name.size()]='\0';
    }
    UI::BeginDisabled(!cameraBankWritable);
    UI::InputText(Label("Camera preset name"),cameraName,sizeof(cameraName));
    if(UI::Button(Label("Save camera preset name"))) {
        auto next=cameraBank;next.names[cameraSlot]=cameraName;
        try {WriteCameraBank(next,cameraBankFile);cameraBank=std::move(next);SetStatus("Camera preset saved: "+std::to_string(cameraSlot+1));}
        catch(const std::exception& e) {SetStatus(e.what());}
    }
    UI::EndDisabled();
    UI::BeginDisabled(cameraSlot==0);
    if(UI::Button(Label("Save current camera to selected slot"))) SaveCameraSlot();
    UI::EndDisabled();
    if(cameraSlot==0) UI::TextUnformatted(Tr("Default camera (fixed)"));
    if(cameraBank[0] && UI::Button(Label("Load previous custom slot 1"))) {
        const auto& s=*cameraBank[0]; distance=s.distance; height=s.height; orbit=s.orbit; pitch=s.pitch; fov=s.fov; lateral=s.lateral; elevation=s.elevation;
        settingsChangedAt=GetTickCount64();
    }


}
void DrawColumnCountIcon(float buttonHeight);
void RenderCameraSlots(bool retakeColumns=false) {
    UI::BeginDisabled(saveBusy);
    // A font-independent settings toggle keeps the toolbar compact in both languages.
    const float side=UI::GetFrameHeight();
    if(UI::GetWindowPos().x+UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x-UI::GetItemRectMax().x>side+UI::GetStyle()->ItemSpacing.x) UI::SameLine();
    if(showCameraSettings) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
    const bool cameraSettingsClicked=UI::Button("##cameraSettings",{side,side});
    if(showCameraSettings) UI::PopStyleColor();
    if(cameraSettingsClicked) showCameraSettings=!showCameraSettings;
    const auto a=UI::GetItemRectMin();
    auto* draw=UI::GetWindowDrawList();
    const auto color=UI::GetColorU32(UI::ImGuiCol_Text);
    const float stroke=std::max(1.f,side*.055f);
    UI::ImDrawListManager::AddRect(draw,{a.x+side*.10f,a.y+side*.30f},{a.x+side*.90f,a.y+side*.82f},color,side*.06f,0,stroke);
    UI::ImDrawListManager::AddLine(draw,{a.x+side*.30f,a.y+side*.30f},{a.x+side*.37f,a.y+side*.17f},color,stroke);
    UI::ImDrawListManager::AddLine(draw,{a.x+side*.37f,a.y+side*.17f},{a.x+side*.63f,a.y+side*.17f},color,stroke);
    UI::ImDrawListManager::AddLine(draw,{a.x+side*.63f,a.y+side*.17f},{a.x+side*.70f,a.y+side*.30f},color,stroke);
    UI::ImDrawListManager::AddCircle(draw,{a.x+side*.50f,a.y+side*.56f},side*.17f,color,20,stroke);
    if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Click to open or close camera settings."));
    for(int n=0;n<7;++n) {
        if(UI::GetWindowPos().x+UI::GetWindowWidth()-UI::GetStyle()->WindowPadding.x-UI::GetItemRectMax().x>UI::GetFrameHeight()+UI::GetStyle()->ItemSpacing.x) UI::SameLine();
        if(n==cameraSlot) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
        const bool clicked=UI::Button((std::to_string(n+1)+"##camera").c_str(),{UI::GetFrameHeight(),0});
        if(n==cameraSlot) UI::PopStyleColor();
        if(UI::IsItemHovered()) {
            if(!cameraBank.names[n].empty()) UI::SetTooltip("%s",cameraBank.names[n].c_str());
            else if(n==0) UI::SetTooltip("%s",Tr("Default camera (fixed)"));
            else UI::SetTooltip(Tr(cameraBank[n]?"Load camera preset %d":"Camera preset %d is empty"),n+1);
        }
        if(clicked) {
            cameraSlot=n;
            if(n==0 || cameraBank[n]) {
                const auto s=n==0 ? StudioSettings{} : *cameraBank[n]; distance=s.distance; height=s.height; orbit=s.orbit; pitch=s.pitch; fov=s.fov; lateral=s.lateral; elevation=s.elevation;
                settingsChangedAt=GetTickCount64();
            } else SetStatus("Empty camera slot: "+std::to_string(n+1));
        }
    }
    if(retakeColumns) {
        const float width=side+20.f;
        const auto* toolbarStyle=UI::GetStyle();
        const float modeWidth=std::max(UI::CalcTextSize(Tr("Normal")).x,UI::CalcTextSize(Tr("Portrait")).x)+toolbarStyle->FramePadding.x*2.f;
        const float groupWidth=width+2.f*modeWidth+2.f*toolbarStyle->ItemSpacing.x;
        const float right=UI::GetCursorPosX()+UI::GetContentRegionAvail().x;
        const float groupX=std::max(UI::GetCursorPosX(),right-groupWidth);
        const float previousRight=UI::GetItemRectMax().x-UI::GetWindowPos().x;
        const char* notice=photoUpdate.Draft().empty()?nullptr:Tr("Photo preview only. Not saved yet.");
        bool noticeDrawn=false;
        if(previousRight+toolbarStyle->ItemSpacing.x*2.f<=groupX) {
            UI::SameLine();
        } else if(notice && UI::GetCursorPosX()+UI::CalcTextSize(notice).x+toolbarStyle->ItemSpacing.x*2.f<=groupX) {
            // Share the folded toolbar row without overlapping translated text.
            UI::AlignTextToFramePadding();
            UI::TextColored({1.f,.8f,.35f,1.f},"%s",notice);
            noticeDrawn=true;
            UI::SameLine();
        }
        UI::SetCursorPosX(groupX);
        if(UI::Button("##retakeColumns",{width,side})) {
            gridColumns=gridColumns==6?2:gridColumns+1;
            // ImGui clamps against the previous frame's content height. Keep
            // revealing the same photo until the new grid's extent has settled.
            photoRetakeReveal=true; photoRetakeLayoutFrames=3;
            SaveSettingsIfDue(true);
        }
        DrawColumnCountIcon(side);
        if(UI::IsItemHovered()) UI::SetTooltip(Tr("Columns: %d (click to cycle)"),ThumbnailColumns(gridColumns,portraitThumbnails));
        for(int mode=0;mode<2;++mode) {
            UI::SameLine();
            const bool selectedMode=portraitThumbnails==(mode==1);
            if(selectedMode) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
            const bool clicked=UI::Button((std::string(Tr(mode?"Portrait":"Normal"))+"##retakeMode").c_str(),{modeWidth,side});
            if(selectedMode) UI::PopStyleColor();
            if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Click to switch between normal and portrait thumbnails."));
            if(clicked && !selectedMode) {
                portraitThumbnails=mode==1; thumbnailModeChanged=true;
                photoRetakeReveal=true; photoRetakeLayoutFrames=3;
                SaveSettingsIfDue(true);
            }
        }
        if(notice && !noticeDrawn) {
            UI::PushStyleColor(UI::ImGuiCol_Text,{1.f,.8f,.35f,1.f});
            UI::TextWrapped("%s",notice);
            UI::PopStyleColor();
        }
    }
    UI::EndDisabled();
    if(showCameraSettings) RenderCameraSettings();
}
#include "MissingCheck.inc"
void DrawColumnCountIcon(float buttonHeight) {
    const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
    const float side=buttonHeight*.28f, gap=buttonHeight*.12f;
    const float x=(a.x+b.x)*.5f-side-gap*.5f, y=(a.y+b.y)*.5f-side-gap*.5f;
    auto* draw=UI::GetWindowDrawList();
    for(int row=0;row<2;++row) for(int col=0;col<2;++col) {
        const UI::ImVec2 p{x+col*(side+gap),y+row*(side+gap)};
        UI::ImDrawListManager::AddRectFilled(draw,p,{p.x+side,p.y+side},0xFFE0E0E0,0,0);
    }
}
void RenderPhotoRetakeControls() {
    captureAllowed=false;
    UI::TextWrapped(Tr("Updating photo: %s"),photoRetakeName.c_str());
    UI::TextWrapped("%s",Tr("Only the photo changes. Saved equipment and automatic outfit assignments stay unchanged."));
    UI::TextWrapped("%s",Tr("The photo uses the current appearance, which may differ from this outfit's saved equipment."));
    const bool review=!photoUpdate.Draft().empty();
    UI::BeginDisabled(saveBusy);
    if(review) {
        if(!saveBusy && (UI::Button(Label("Use this photo")) || PadPressed(UI::ImGuiKey_GamepadFaceDown,false))) {
            try {
                // Invalidate before the next draw; never release a texture referenced by this frame.
                DisposePortraitView(photoUpdate.Target().generic_string());
                photoUpdate.Commit();
                CancelPhotoRetake();
                SetStatus("Photo updated. Equipment and assignments unchanged.");
                framePadEdges=framePadRepeat=0;
            } catch(const std::exception& e) {SetStatus(e.what());}
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s / A",Tr("Use this photo"));
        UI::SameLine();
    }
    const bool shoot=UI::Button(Label(review?"Retake":"Take photo")) || RawPadPressed(capturePads[0]);
    if(UI::IsItemHovered()) UI::SetTooltip(Tr("Capture / retake: %s"),CaptureHint(0).c_str());
    UI::SameLine();
    const bool cancel=UI::Button(Label("Cancel")) || PadPressed(UI::ImGuiKey_GamepadFaceRight,false);
    if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("B: cancel"));
    UI::EndDisabled();
    if(photoRetakeActive && !saveBusy && cancel) {
        CancelPhotoRetake(); SetStatus("Photo update cancelled. Original photo kept.");
        framePadEdges=framePadRepeat=0;
    } else if(photoRetakeActive && !saveBusy && shoot) {
        saveBusy=true; photoRetakeCaptureRequested=true;
    }
    if(photoRetakeActive) {
        RenderCameraSlots(true);
    }
}
#include "PresetEditUI.inc"
#include "EquipmentRemovalUI.inc"
#include "GalleryBrowser.inc"
#include "AutoOutfitUI.inc"

void __stdcall RenderStudio() {
    if(photoRetakeActive && (!active || photoRetakeSerial!=studioOpenSerial.load())) CancelPhotoRetake();
    DrainOrganizerTextures();
    organizerFrame=false;
    ProcessCapturedHotkey();
    static char name[160]{};
    UpdatePadFrame();
    if(framePadEdges || UI::IsKeyDown(UI::ImGuiKey_GamepadLStickUp) || UI::IsKeyDown(UI::ImGuiKey_GamepadLStickDown) ||
       UI::IsKeyDown(UI::ImGuiKey_GamepadLStickLeft) || UI::IsKeyDown(UI::ImGuiKey_GamepadLStickRight)) galleryPadPointer=true;
    // SMF shares its ImGui context. Suppress hover only during this callback and
    // restore IO before another mod draws; do not warp the OS cursor or set global NoMouse.
    const bool hideGalleryPointer=galleryPadPointer.load();
    PointerPositionScope pointerPosition(*UI::GetIO(),hideGalleryPointer);
    struct CursorScope {
        bool hidden;
        ~CursorScope() {if(hidden) UI::SetMouseCursor(UI::ImGuiMouseCursor_None);}
    } cursorScope{hideGalleryPointer};
    if(UI::IsKeyPressed(UI::ImGuiKey_Escape,false)) GALLERY_DIAG("OG-OPEN-DIAG UI Escape");
    static unsigned openedSerial{};


    if(openedSerial!=studioOpenSerial.load()) {openedSerial=studioOpenSerial.load(); showCameraSettings=false; autoPanel=false;}


    const auto display = UI::GetIO()->DisplaySize;
    const auto framebuffer = UI::GetIO()->DisplayFramebufferScale;
    const auto surface = ReadLayoutSurface();
    const auto layout = ResolveLayoutSize({display.x,display.y},{framebuffer.x,framebuffer.y},surface);
    const UI::ImVec2 screen{layout.width,layout.height};
    // Shared style/font scale is restored at callback exit, including early returns.
    ScopedGalleryScale scaleScope(screen,uiZoom);
    static unsigned diagnosticSerial{}, diagnosticSample{};
    static ULONGLONG diagnosticDue{};
    if(diagnosticSerial!=studioOpenSerial.load()) {
        diagnosticSerial=studioOpenSerial.load(); diagnosticSample=0; diagnosticDue=0;
    }
    const bool diagnosticNow=diagnosticSample<3 && GetTickCount64()>=diagnosticDue;
    UI::SetNextWindowPos({screen.x*0.01f, screen.y*0.015f}, UI::ImGuiCond_Always);
    UI::SetNextWindowSize({screen.x*0.515f, screen.y*0.97f}, UI::ImGuiCond_Always);
    bool opened = true;
    if (UI::Begin("Outfit Gallery", &opened, UI::ImGuiWindowFlags_NoResize | UI::ImGuiWindowFlags_NoMove | UI::ImGuiWindowFlags_NoCollapse | UI::ImGuiWindowFlags_NoNavInputs | UI::ImGuiWindowFlags_NoTitleBar | UI::ImGuiWindowFlags_NoScrollbar | UI::ImGuiWindowFlags_NoScrollWithMouse)) {
        if(photoRetakeActive) {
            const auto titlePos=UI::GetCursorPos();
            const float closeSide=UI::GetFrameHeight();
            const float closeX=titlePos.x+UI::GetContentRegionAvail().x-closeSide;
            UI::AlignTextToFramePadding(); UI::TextUnformatted("Outfit Gallery");
            UI::SetCursorPos({closeX,titlePos.y});
            if(UI::Button("X##closePhotoRetake",{closeSide,closeSide})) opened=false;
            UI::Separator();
        } else RenderGalleryTitle(opened);
        if(diagnosticNow) {
            ++diagnosticSample; diagnosticDue=GetTickCount64()+1500;
            const auto size=UI::GetWindowSize(), pos=UI::GetWindowPos();
            SKSE::log::info("UI-SCALE-TEST1 open={} sample={} display={}x{} framebuffer={}x{} swap={}x{} buffer={}x{} viewport={}x{} valid={} layout={}x{} corrected={} actualWindow={}x{} position={},{} font={} detached={} uiScale={} uiZoom={}",
                diagnosticSerial,diagnosticSample,display.x,display.y,framebuffer.x,framebuffer.y,
                surface.swap.width,surface.swap.height,surface.buffer.width,surface.buffer.height,
                surface.viewport.width,surface.viewport.height,surface.valid,screen.x,screen.y,
                screen.x!=display.x || screen.y!=display.y,size.x,size.y,pos.x,pos.y,UI::GetFontSize(),detachedPreview.load(),uiFrameScale,uiZoom);
        }
        UI::BeginChild("Gallery contents",{0,0},0,UI::ImGuiWindowFlags_NoNavInputs);
        if(photoRetakeActive) RenderPhotoRetakeControls();
        else {
        float targetRowRight=-1.f;
        if(studioFollower) {
            std::string targetName; {std::scoped_lock lock(statusMutex); targetName=studioActorName;}
            UI::Text(Tr("Follower: %s"),targetName.c_str());
            UI::BeginDisabled(saveBusy || verificationTicks.load()!=0 || FollowerAutomaticOwns(studioActorID.load()));
            bool keep=followerMaintained.load();
            if(UI::Checkbox(Tr("Maintain this follower's outfit"),&keep)) command=keep?7:8;
            if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Checks every 5 seconds while closed. Missing items or repeated conflicts disable maintenance."));
            UI::SameLine();
            if(UI::Button(Tr("Restore standard outfit"))) command=9;
            targetRowRight=UI::GetItemRectMax().x-UI::GetWindowPos().x;
            UI::EndDisabled();
            if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Disables maintenance and restores standard armor. Given items and weapons are kept."));
        }
        if(!studioFollower && followerTargeting) {
            UI::TextUnformatted(Tr("Target: Player"));
            targetRowRight=UI::GetItemRectMax().x-UI::GetWindowPos().x;
        }
        const auto* rowStyle=UI::GetStyle();
        const float modeWidth=std::max(UI::CalcTextSize(Tr("Normal")).x,UI::CalcTextSize(Tr("Portrait")).x)+rowStyle->FramePadding.x*2.f;
        const float nameLabelWidth=UI::CalcTextSize(Tr("Preset name")).x;
        const float nameGap=std::max(rowStyle->ItemSpacing.x*2.f,UI::GetFontSize()*.65f);
        // Reserve the right-hand controls first. Only the name field stretches,
        // leaving a deliberate gap after its label at both FHD and WQHD.
        const float rowStart=UI::GetCursorPosX();
        const float rowRight=rowStart+UI::GetContentRegionAvail().x;
        const float helpX=rowRight-UI::GetFrameHeight();
        const float modeX=helpX-rowStyle->ItemSpacing.x*2.f-modeWidth*2.f;
        if(!compactGallery) {
            UI::SetNextItemWidth(std::max(32.f,modeX-nameGap-nameLabelWidth-rowStyle->ItemInnerSpacing.x-rowStart));
            UI::InputText(Label("Preset name"),name,sizeof(name));
            UI::SameLine();
        } else {
            const float organizeWidth=UI::CalcTextSize(Tr("Organize gallery")).x+rowStyle->FramePadding.x*2.f;
            const float organizeX=std::max(rowStart,modeX-rowStyle->ItemSpacing.x-organizeWidth);
            // Use measured item bounds: translations and font scaling may need a second row.
            if(targetRowRight>=0.f && targetRowRight+rowStyle->ItemSpacing.x*2.f<=organizeX)
                UI::SameLine();
            UI::SetCursorPosX(organizeX);
            UI::BeginDisabled(saveBusy);
            if(UI::Button(Label("Organize gallery"))) compactOrganizeRequested=true;
            if(missingCheckFocused) UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
            UI::EndDisabled();
            UI::SameLine();
        }
        UI::SetCursorPosX(modeX);
        UI::BeginDisabled(saveBusy);
        for(int mode=0;mode<2;++mode) {
            if(mode) UI::SameLine();
            const bool selectedMode=portraitThumbnails==(mode==1);
            const auto modeLabel=std::string(Tr(mode?"Portrait":"Normal"))+(mode?"###portraitMode":"###normalMode");
            if(selectedMode) UI::PushStyleColor(UI::ImGuiCol_Button,{.55f,.43f,.19f,1.f});
            const bool pressed=UI::Button(modeLabel.c_str(),{modeWidth,0});
            if(selectedMode) UI::PopStyleColor();
            if(layoutButtonFocused && selectedMode) UI::ImDrawListManager::AddRect(UI::GetWindowDrawList(),UI::GetItemRectMin(),UI::GetItemRectMax(),0xFF30C8FF,0,0,3.f);
            if(UI::IsItemHovered()) UI::SetTooltip("%s",Tr("Click to switch between normal and portrait thumbnails."));
            if(pressed && !selectedMode) {
                portraitThumbnails=mode==1; thumbnailModeChanged=true; SaveSettingsIfDue(true);
            }
        }
        UI::EndDisabled();
        UI::SameLine(); UI::SetCursorPosX(helpX);
        if(UI::Button("?##help",{UI::GetFrameHeight(),0})) helpOpen=!helpOpen;

        UI::BeginDisabled(saveBusy);

        if (UI::Button(Label("Save outfit"))) {
            if (!saveBusy.exchange(true)) {
                { std::scoped_lock lock(presetsMutex); requestedName = compactGallery ? "" : name; }
                command = 3;
            }
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s\n%s",Tr("Register worn armor and clothing. Applying keeps weapons and ammunition."),CaptureHint(0).c_str());
        UI::EndDisabled();
        UI::SameLine();
        UI::BeginDisabled(saveBusy);
        if(UI::Button(Label("Save headgear")) && !saveBusy.exchange(true)) {
            {std::scoped_lock lock(presetsMutex); requestedName=compactGallery ? "" : name;} command=5;
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s\n%s",Tr("Register head equipment. Applying keeps clothes and weapons."),CaptureHint(1).c_str());
        UI::EndDisabled();
        UI::SameLine();
        UI::BeginDisabled(saveBusy);
        if(UI::Button(Label("Save accessories")) && !saveBusy.exchange(true)) {
            {std::scoped_lock lock(presetsMutex); requestedName=compactGallery ? "" : name;} command=6;
        }
        if(UI::IsItemHovered()) UI::SetTooltip("%s\n%s",Tr("Wear only the accessories to register. Applying adds or replaces their slots."),CaptureHint(2).c_str());
        UI::EndDisabled();
        RenderCameraSlots();
        UI::SameLine();
        const float removeAvailable=UI::GetContentRegionAvail().x;
        const float removeTextWidth=UI::CalcTextSize(Tr("Remove equipment")).x+UI::GetStyle()->FramePadding.x*2.f;
        const bool compactRemoval=removeAvailable<removeTextWidth;
        const float removalWidth=compactRemoval?UI::GetFrameHeight():removeTextWidth;
        UI::SetCursorPosX(UI::GetCursorPosX()+std::max(0.f,removeAvailable-removalWidth));
        RenderEquipmentRemoval(compactRemoval);
        }
        if (showEquipmentTools && !autoPanel && !photoRetakeActive && !studioFollower) RenderRandomTryOn();
        std::string text, image;
        { std::scoped_lock lock(statusMutex); text = status; image = lastImage; }
        if(!text.empty() && !(photoRetakeActive && text=="Photo preview only. Not saved yet.") && text!="Adjust composition and press F9/Y to save." && text!="Save equipment with F9; click a photo to apply a preset.") UI::TextWrapped(Tr("%s"), StatusText(text).c_str());
        UI::Separator();
        if(autoPanel) UI::TextWrapped("%s",Tr("Select or drag a photo to assign it. Clicking photos here does not equip them."));
        UI::PushStyleVar(UI::ImGuiStyleVar_DisabledAlpha,1.f);
        UI::BeginDisabled(photoRetakeActive);
        RenderBrowser(addMissing);
        UI::EndDisabled();
        UI::PopStyleVar();
        UI::EndChild();
    }
    UI::End();
    if(organizerFrame) return; // Modal owns Escape/pad and hides the preview/auto panel.
    if(helpOpen) {
        UI::SetNextWindowSize({screen.x*.40f,screen.y*.60f},UI::ImGuiCond_FirstUseEver);
        if(UI::Begin(Label("Help"),&helpOpen,UI::ImGuiWindowFlags_NoCollapse)) {
            UI::TextWrapped(Tr("1. Wear the items to register, then use a registration button or its assigned key."));
            UI::TextWrapped(Tr("Outfit: register worn armor and clothing; weapons and ammunition are excluded."));
            UI::Text("%s",CaptureHint(0).c_str());
            UI::TextWrapped(Tr("Head: register head equipment only."));
            UI::Text("%s",CaptureHint(1).c_str());
            UI::TextWrapped(Tr("Accessories: wear only the items you want to add, then register."));
            UI::Text("%s",CaptureHint(2).c_str());
            UI::Spacing(); UI::Spacing();
            UI::TextWrapped(Tr("Select a photo below to apply its equipment."));
            UI::TextWrapped(Tr("Text tabs: replace the full outfit (organize with tabs)."));
            UI::TextWrapped(Tr("Person icon: change only head equipment, keeping your clothes."));
            UI::TextWrapped(Tr("Ring icon: add the pictured items, keeping your clothes; replace items in matching slots."));
            UI::Spacing();
            UI::TextWrapped(Tr("D-pad: select. A: apply. LB/RB: tabs. LT: head. RT: accessories. R3: photo menu. B: close."));
            UI::TextWrapped(Tr("Enable Add missing base items to restore unowned items. Conflicting worn armor is removed as a whole, including wigs or clothing using multiple slots."));
            UI::TextWrapped(Tr("Drag inside the camera frame to move the subject. Use the wheel to change distance."));
            UI::TextWrapped(Tr("Right-drag horizontally to orbit; vertically to look up or down."));
            if(UI::Button(Label("Close"))) helpOpen=false;
        }
        UI::End();
        if(RawPadPressed(8192)) helpOpen=false;
    }
    if(autoPanel) {RenderAutoOutfitPanel(screen); if(!opened || UI::IsKeyPressed(UI::ImGuiKey_Escape,false)) command=2; return;}
    const bool preview=detachedPreview.load();
    bool previewOpen=true;
    static UI::ImVec2 lastScreen{};
    if(lastScreen.x!=screen.x || lastScreen.y!=screen.y) {resetPreviewRect=true; lastScreen=screen;}
    auto flags=UI::ImGuiWindowFlags_NoSavedSettings|UI::ImGuiWindowFlags_NoNav|UI::ImGuiWindowFlags_NoScrollWithMouse|UI::ImGuiWindowFlags_NoScrollbar;
    if(preview) {
        UI::SetNextWindowSizeConstraints({160,160},{screen.x,screen.y});
        if(resetPreviewRect) {
            const float w=std::clamp(previewRect[2]*screen.x,160.f,screen.x);
            const float h=std::clamp(previewRect[3]*screen.y,160.f,screen.y);
            UI::SetNextWindowPos({std::clamp(previewRect[0]*screen.x,0.f,screen.x-w),std::clamp(previewRect[1]*screen.y,0.f,screen.y-h)},UI::ImGuiCond_Always);
            UI::SetNextWindowSize({w,h},UI::ImGuiCond_Always);
            resetPreviewRect=false;
        }
        flags|=UI::ImGuiWindowFlags_NoCollapse;
        UI::SetNextWindowBgAlpha(1.f);
    } else {
        UI::SetNextWindowPos({screen.x*.54f,screen.y*.08f},UI::ImGuiCond_Always);
        UI::SetNextWindowSize({screen.x*.45f,screen.y*.84f},UI::ImGuiCond_Always);
        flags|=UI::ImGuiWindowFlags_NoDecoration|UI::ImGuiWindowFlags_NoMove|UI::ImGuiWindowFlags_NoBackground|UI::ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
    UI::PushStyleVar(UI::ImGuiStyleVar_WindowPadding,{0,0});
    if(UI::Begin(preview?Label("Live preview"):"##cameraMouse",preview?&previewOpen:nullptr,flags)) {
        auto available=UI::GetContentRegionAvail();
        UI::ImVec2 imageSize=available;
        float mouseScale=1.f;
        if(preview) {
            const auto pos=UI::GetWindowPos(), size=UI::GetWindowSize();
            std::array<float,4> rect{std::clamp(pos.x/screen.x,0.f,1.f),std::clamp(pos.y/screen.y,0.f,1.f),std::clamp(size.x/screen.x,.1f,1.f),std::clamp(size.y/screen.y,.1f,1.f)};
            if(rect!=previewRect) {previewRect=rect; settingsChangedAt=GetTickCount64();}
            auto cursor=UI::GetCursorScreenPos();
            const auto pixels=LivePreviewSize();
            const auto fb=UI::GetIO()->DisplayFramebufferScale;
            const auto layout=FitPreview(cursor.x,cursor.y,available.x,available.y,
                pixels[0]?float(pixels[0]):screen.x*.45f,
                pixels[1]?float(pixels[1]):screen.y*.84f,fb.x,fb.y);
            imageSize={layout.width,layout.height};
            cursor={layout.x,layout.y};
            UI::SetCursorScreenPos(cursor);
            if(auto view=LivePreviewView(); view && previewSettled>=3)
                UI::ImDrawListManager::AddImage(UI::GetWindowDrawList(),view,cursor,{cursor.x+imageSize.x,cursor.y+imageSize.y},{0,0},{1,1},0xFFFFFFFF);
            mouseScale=screen.x*.45f/std::max(1.f,imageSize.x);
        }
        UI::InvisibleButton("##pan",{std::max(1.f,imageSize.x),std::max(1.f,imageSize.y)},UI::ImGuiButtonFlags_MouseButtonLeft|UI::ImGuiButtonFlags_MouseButtonRight);
        if(portraitThumbnails && !saveBusy && thumbnailVisibleWidth<1.f) {
            const auto a=UI::GetItemRectMin(), b=UI::GetItemRectMax();
            const float inset=(b.x-a.x)*(1.f-thumbnailVisibleWidth)*.5f;
            auto* guide=UI::GetWindowDrawList();
            UI::ImDrawListManager::AddLine(guide,{a.x+inset,a.y},{a.x+inset,b.y},0xCCEEE09C,1.f);
            UI::ImDrawListManager::AddLine(guide,{b.x-inset,a.y},{b.x-inset,b.y},0xCCEEE09C,1.f);
        }
        if(!saveBusy && !helpOpen && !bindingMode) {
            auto* io=UI::GetIO();
            const float units=2.f*distance.load()*std::tan(fov.load()*.00872664626f)/std::max(1.f,screen.x)*mouseScale;
            bool changed=false;
            if(UI::IsItemActive() && UI::IsMouseDragging(0) && !UI::IsMouseDown(1)) {
                lateral=std::clamp(lateral.load()+io->MouseDelta.x*units,-150.f,150.f);
                height=std::clamp(height.load()+io->MouseDelta.y*units,-20.f,180.f);
                changed=io->MouseDelta.x!=0 || io->MouseDelta.y!=0;
            }
            if(UI::IsItemActive() && UI::IsMouseDragging(1) && !UI::IsMouseDown(0)) {
                // Screen-relative sensitivity keeps the same gesture consistent
                // across resolutions. Wrap orbit continuously; limit elevation
                // to the same range as its slider.
                orbit=std::remainder(orbit.load()+io->MouseDelta.x*mouseScale*180.f/std::max(1.f,screen.x),360.f);
                elevation=std::clamp(elevation.load()+io->MouseDelta.y*mouseScale*180.f/std::max(1.f,screen.y),-60.f,60.f);
                changed=io->MouseDelta.x!=0 || io->MouseDelta.y!=0;
            }
            if(UI::IsItemHovered() && io->MouseWheel!=0) {
                distance=std::clamp(distance.load()*std::pow(.9f,io->MouseWheel),10.f,400.f); changed=true;
            }
            if(changed) settingsChangedAt=GetTickCount64();
        }
    }
    UI::End(); UI::PopStyleVar();
    if(preview && !previewOpen && !saveBusy) {detachedPreview=false; previewSettled=0; settingsChangedAt=GetTickCount64();}
    auto* draw = UI::GetForegroundDrawList();
    if(!preview) UI::ImDrawListManager::AddRect(draw, {screen.x*.54f,screen.y*.08f}, {screen.x*.99f,screen.y*.92f}, 0xFF9BC6E8, 0, 0, 2);
    if (!opened || UI::IsKeyPressed(UI::ImGuiKey_Escape, false)) command = 2;
}

void __stdcall RenderSettings() {
    ProcessCapturedHotkey();
    RenderHotkeys();
    UI::TextUnformatted("Outfit Gallery 2.2.0");
    UI::TextWrapped(Tr("Close this menu and press the configured hotkey (default F8) while standing in a safe open area."));
    UI::TextUnformatted(Tr("F9 in the studio saves a photo and equipment. Click a preset photo to apply."));
    UI::Text(Tr("SmoothCam API: %s / TDM API: %s"), smooth ? "ready" : "not available", tdm ? "ready" : "not available");
    std::scoped_lock lock(statusMutex);
    UI::TextWrapped(Tr("%s"), StatusText(status).c_str());
}

bool __stdcall Input(RE::InputEvent* event) {

    if (!event || !ready) return false;
    if(const auto* mouse=event->AsMouseMoveEvent();mouse && (mouse->mouseInputX || mouse->mouseInputY)) galleryPadPointer=false;
    auto* button = event->AsButtonEvent();
    if (!button) return false;
    const auto device=button->GetDevice();
    const auto code=button->GetIDCode();
    if(device==RE::INPUT_DEVICE::kMouse && button->IsPressed()) galleryPadPointer=false;
    if(device==RE::INPUT_DEVICE::kGamepad && button->IsDown()) galleryPadPointer=true;
    const unsigned binding=bindingMode.load();
    if(binding && GetTickCount64()<=bindingDeadline && button->IsDown()) {
        if(device==RE::INPUT_DEVICE::kKeyboard && code==1) {bindingMode=0; return true;}
        if(((binding%2)==1 && device==RE::INPUT_DEVICE::kKeyboard) || ((binding%2)==0 && device==RE::INPUT_DEVICE::kGamepad)) {
            if((binding==1 || binding==9) && HotkeyChord::ModifierForKey(code)) return true;
            capturedBinding=HotkeyChord::Pack(binding,code,(binding==1 || binding==9)?HeldKeyboardModifiers():0u); bindingMode=0; padHold={};autoPadHold={}; return true;
        }
    }
    if(device==RE::INPUT_DEVICE::kGamepad) {
        const unsigned bit=code==9?0x10000u:code==10?0x20000u:code;
        if(button->IsPressed()) physicalPadHeld.fetch_or(bit); else physicalPadHeld.fetch_and(~bit);
        if(button->IsDown() && active) padEdges.fetch_or(bit);
        if(button->IsDown() && active) padHeld.fetch_or(bit); else if(!button->IsPressed()) padHeld.fetch_and(~bit);
    }
    if(device==RE::INPUT_DEVICE::kGamepad && autoGamepad && code==autoGamepad) {
        if(autoPadHold.Update(button->IsDown(),button->IsPressed(),button->HeldDuration(),autoHoldSeconds.load())) {autoToggleRequested=true;}
        return false;
    }
    if(device==RE::INPUT_DEVICE::kGamepad && gamepadHotkey && code==gamepadHotkey) {
        if(padHold.Update(button->IsDown(),button->IsPressed(),button->HeldDuration(),padHoldSeconds.load())) {
            GALLERY_DIAG("OG-OPEN-DIAG hold-fired code={} active={} duration={}",code,active.load(),button->HeldDuration());
            if(!active) openingPadBlock=code==9?0x10000u:code==10?0x20000u:code;
            command=active?2:1; QueueTick();
        }
        // Observe, never consume: short presses and release reach the game/other mods.
        return false;
    }
    // Start is a gallery shortcut only when no user binding owns it.
    if(device==RE::INPUT_DEVICE::kGamepad && code==16 && active &&
       gamepadHotkey!=16 && autoGamepad!=16 && capturePads[0]!=16 && capturePads[1]!=16 && capturePads[2]!=16) return true;
    if(device!=RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) return false;
    if(autoKeyboard && code==autoKeyboard && HotkeyChord::Matches(autoModifiers.load(),HeldKeyboardModifiers() & ~HotkeyChord::ModifierForKey(code))) {
        autoToggleRequested=true;return true;
    }
    if(active && photoRetakeActive && captureKeys[0] && code==captureKeys[0]) {
        if(!saveBusy.exchange(true)) photoRetakeCaptureRequested=true;
        return true;
    }
    for(unsigned action=0;action<3;++action) if (active && !photoRetakeActive && !editPending && captureAllowed && captureKeys[action] && code==captureKeys[action]) {
        if (!saveBusy.exchange(true)) {
            { std::scoped_lock lock(presetsMutex); requestedName.clear(); }
            command = action==0?3:action==1?5:6; QueueTick();
        }
        return true;
    }
    if(active && code==1 && organizerInputActive.load()) return false; // Let the organizer handle Escape without closing the studio.
    if ((code == hotkey && HotkeyChord::Matches(keyboardModifiers.load(),HeldKeyboardModifiers() & ~HotkeyChord::ModifierForKey(code))) || (active && code == 1)) {
        GALLERY_DIAG("OG-OPEN-DIAG keyboard-toggle code={} active={}",code,active.load());
        if(!active) galleryPadPointer=false;
        command = active ? 2 : 1;
        // Closing must also work if the next UI render callback never arrives.
        QueueTick();
        return true;
    }
    return false;
}

void Message(SKSE::MessagingInterface::Message* message) {
    if (!message) return;
    switch (message->type) {
    case SKSE::MessagingInterface::kPostLoad:
        (void)SmoothCamAPI::RegisterInterfaceLoaderCallback(SKSE::GetMessagingInterface(), [](void* api, SmoothCamAPI::InterfaceVersion v) {
            if (v == SmoothCamAPI::InterfaceVersion::V3) smooth = static_cast<SmoothCamAPI::IVSmoothCam3*>(api);
        });
        if (GetModuleHandleW(L"TrueDirectionalMovement.dll")) tdm = static_cast<TDM_API::IVTDM2*>(TDM_API::RequestPluginAPI(TDM_API::InterfaceVersion::V2));
        break;
    case SKSE::MessagingInterface::kPostPostLoad:
        (void)SmoothCamAPI::RequestInterface(SKSE::GetMessagingInterface());
        break;
    case SKSE::MessagingInterface::kDataLoaded: {
        InstallTransmog();
        LoadExternalTranslation();
        RegisterManagedItemEvents();
        InitializeOStimCompatibility();
        InitializeGuildLocations();
        RegisterCraftEvents();
        RegisterFollowerArrivalDiagnostics();
        if (!GetMenuFrameworkModule() || !GetProcAddress(GetMenuFrameworkModule(), "RegisterEventPriority") || !GetProcAddress(GetMenuFrameworkModule(), "RegisterInpoutEvent")) {
            SetStatus("A compatible SKSE Menu Framework is required."); break;
        }
        window = SKSEMenuFramework::AddWindow(RenderStudio, true);
        if (!window) { SetStatus("Could not create framework window."); break; }
        noticeWindow = SKSEMenuFramework::AddWindow(RenderNotice, false);
        SKSEMenuFramework::SetSection("Outfit Gallery");
        SKSEMenuFramework::AddSectionItem("Photo presets", RenderSettings);
        (void)SKSEMenuFramework::AddEvent(OnFrame, 0);
        (void)SKSEMenuFramework::AddInputEvent(Input);
        try {
            if(std::filesystem::exists(settingsFile)) {
                const auto s=ReadStudioSettings(settingsFile);
                distance=s.distance; height=s.height; orbit=s.orbit; pitch=s.pitch; fov=s.fov; lateral=s.lateral; elevation=s.elevation; gridColumns=s.columns; showNames=s.showNames; showCounts=s.showCounts; language=s.language; allowFreeCamera=s.allowFreeCamera; addMissing=s.addMissing; partialSlots=s.headSlots; startupTab=s.startupTab; detachedPreview=s.detachedPreview; previewRect=s.previewRect; followerTargeting=s.followerTargeting; preferEnchanted=s.preferEnchanted; portraitThumbnails=s.portraitThumbnails; cleanupCompatibility=s.cleanupCompatibility; allowCombatGallery=s.allowCombatGallery; compactGallery=s.compactGallery; reclaimRenamedGeneratedItems=s.reclaimRenamedGeneratedItems; uiZoom=s.uiZoom; fullOutfitToggle=s.fullOutfitToggle; upscalerCompatibility=s.upscalerCompatibility; showEquipmentTools=s.showEquipmentTools;
                SKSE::log::info("Loaded persistent studio settings");
                if(s.followerTargetingMigrationPending) {
                    try {
                        WriteStudioSettings(s,settingsFile);
                        SKSE::log::info("Follower targeting enabled by one-time default migration (version 1)");
                    } catch(const std::exception& e) {
                        SKSE::log::warn("Could not persist follower targeting migration: {}",e.what());
                        settingsChangedAt=GetTickCount64();
                    }
                }
            }
        } catch(const std::exception& e) { SetStatus(std::string("Camera settings: ")+e.what()); }
        try {
            if(std::filesystem::exists(libraryFile)) library=ReadLibrary(libraryFile);
            else library.categories={{"favorites","Favorites"},{"everyday","Everyday"},{"battle","Battle"},{"dress","Custom"}};
        } catch(const std::exception& e) { libraryWritable=false; SetStatus(std::string("Library read failed: ")+e.what()); }
        try {
            if(std::filesystem::exists(inputSettingsFile)) {
                auto keys=ReadInputSettings(inputSettingsFile);
                autoKeyboard=keys.autoKeyboard;autoModifiers=keys.autoModifiers;autoGamepad=keys.autoGamepad;autoHoldSeconds=keys.autoHoldSeconds;
                hotkey=keys.keyboard; keyboardModifiers=keys.keyboardModifiers; gamepadHotkey=keys.gamepad; padHoldSeconds=keys.holdSeconds;
                for(unsigned n=0;n<3;++n) {captureKeys[n]=keys.captureKeys[n];capturePads[n]=keys.capturePads[n];}
            }
        }catch(const std::exception& e){SetStatus(std::string("Hotkey settings: ")+e.what());}
        try {if(std::filesystem::exists(cameraBankFile)) cameraBank=ReadCameraBank(cameraBankFile);}
        catch(const std::exception& e){cameraBankWritable=false; SetStatus(e.what());}
        try {if(std::filesystem::exists(autoFile)) {
            const auto settings=ReadAutoOutfitSettings(autoFile);
            autoHideAssignButtons=settings.hideAssignButtons; autoSlots=settings.slots; autoDelaySeconds=settings.delaySeconds; autoSneakDelaySeconds=settings.sneakDelaySeconds; autoCombatIntervalSeconds=settings.combatIntervalSeconds; autoColumns=settings.columns; ostimRecoverySeconds=settings.ostimRecoverySeconds;
            autoEnabled=settings.enabled; autoPaused=settings.paused; autoCombatEnabled=settings.combatEnabled;
        }}
        catch(const std::exception& e) {autoSlotsWritable=false; SetStatus(e.what());}
        ready = true;
        try { ReloadPresets(); } catch (const std::exception& e) { SetStatus(e.what()); }
        SKSE::log::info("Registered gallery UI; SmoothCam={}, TDM={}", smooth != nullptr, tdm != nullptr);
        break;
    }
    case SKSE::MessagingInterface::kPreLoadGame:
        ClearAllTransmog();
        ++epoch; autoToggleRequested=false; command = 0; verificationTicks=0; ResetAutoOutfits(); Close(); ready = false; break;
    case SKSE::MessagingInterface::kPostLoadGame:
        ClearAllTransmog();
        ++epoch; physicalPadHeld=0; openingPadBlock=0; ResetGalleryPadInput("postload"); autoToggleRequested=false; command=0; verificationTicks=0; ResetAutoOutfits();
        if(message->data) StartTransmogRestore(); else ResetTransmogPersistence();
        ready=window!=nullptr; break;
    case SKSE::MessagingInterface::kNewGame:
        ResetTransmogPersistence();
        ClearAllTransmog();
        ++epoch; physicalPadHeld=0; openingPadBlock=0; ResetGalleryPadInput("postload/newgame"); autoToggleRequested=false; command = 0; verificationTicks=0; ResetAutoOutfits(); ready = window != nullptr; break;
    default: break;
    }
}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    if (skse->IsEditor()) return false;
    const auto logs = SKSE::log::log_directory();
    if (!logs) return false;
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*logs / "OutfitGallery.log").string(), true);
    auto logger = std::make_shared<spdlog::logger>("OutfitGallery", sink);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
    SKSE::log::info("OutfitGallery release=2.2.0; equipment acquisition, random try-on, customization and reroll; 2.0.1 compatibility fixes retained");
    const auto runtime=skse->RuntimeVersion();
    const auto supported=SKSEPlugin_Version.GetRuntimeCompatibility().GetCompatibleRuntimeVersions();
    if(std::none_of(supported.begin(),supported.end(),[&](const auto& version){return static_cast<REL::Version>(version)==runtime;})) {
        SKSE::log::error("Unsupported Skyrim runtime {}. Supported targets: 1.5.97, 1.6.353, 1.6.640, 1.6.1130, 1.6.1170, GOG 1.6.1179, Steam 1.7.104. VR is not enabled.",runtime.string());
        return false;
    }
    SKSE::log::info("Runtime {} accepted; public-version target list retained; RUNTIME4 transmog requires callsite validation; per-runtime gameplay tests pending; GOG 1.6.1179 untested.",runtime.string());
    SKSE::Init(skse, false);
    Gallery::InitializeManagedItems();
    const auto ini = std::filesystem::absolute("Data/SKSE/Plugins/OutfitGallery.ini");
    Gallery::hotkey = GetPrivateProfileIntW(L"Input", L"Hotkey", 66, ini.c_str());
    Gallery::saveHotkey = GetPrivateProfileIntW(L"Input", L"SaveHotkey", 67, ini.c_str());
    Gallery::captureKeys[0]=Gallery::saveHotkey;
    Gallery::gamepadHotkey = GetPrivateProfileIntW(L"Input", L"GamepadHotkey", 32, ini.c_str());
    SKSE::log::info("OutfitGallery 2.2.0");
    SKSE::log::info("OutfitGallery 2.2.0 (CommonLib 10.1.0); runtime {}; key={}", runtime.string(), Gallery::hotkey.load());
    return SKSE::GetMessagingInterface()->RegisterListener(Gallery::Message);
}








