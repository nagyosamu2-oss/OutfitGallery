# Outfit Gallery 2.0.1

A photo-based outfit gallery with normal equipment changes and appearance-only transmog for the player and followers.

## Changes in 2.0.1
- Added Upscaler compatibility mode, OFF by default. Enable it in Options if upscaling makes Gallery text blurry or enlarged, then close and reopen Gallery. The choice is saved across game restarts.
- In compatibility mode, added HUD widgets that respond to Cursor Menu can hide while Gallery is open and return afterward. The native game cursor is hidden during Gallery to prevent overlapping pointers.
- Integrated the Steam 1.7.104 Transmog Fix. Verified fallback call positions can be used when normal call-target discovery cannot locate the required hooks.
- Fixed Gallery entry when another mod already owns TDM's headtracking-disable control. This resolves the reported first-person entry failure with ImprovedCameraSE 2.0.0.2231. External ownership is respected.

## Requirements
Windows x64, the SKSE version matching your executable, Address Library, and SKSE Menu Framework 3 with its dependencies. A suitable framework font is needed for Japanese. Disable the framework's Blur Background option for a clear preview. SmoothCam and True Directional Movement are optional. VR is not supported. Third-party framework DLLs, fonts, outfits and game assets are not bundled.

Accepted targets: Steam 1.5.97 / 1.6.353 / 1.6.640 / 1.6.1130 / 1.6.1170 / 1.7.104; GOG 1.6.1179.
Development testing includes Steam 1.5.97 and 1.6.1170, plus the 1.7.104 transmog fix tested by the user. Other accepted runtimes have implementation coverage and remain untested in gameplay for these changes. GOG 1.6.1179 is untested. See VALIDATION.md.

## Install or update
Close Skyrim and install this complete archive with your mod manager. It contains the DLL and default INI; no ESP or Papyrus scripts are required.
Keep generated Data/SKSE/Plugins/OutfitGallery data: photos, presets and JSON settings. Preserve a customized OutfitGallery.ini. With MO2, check the existing mod folder, Overwrite and any output mod before replacing files. Disable older trial DLL overrides and the standalone 1.7.104 Transmog Fix so the 2.0.1 DLL takes priority. Hotkeys.json takes priority over INI hotkeys. A new game or preset re-registration is not required.
The first load of legacy settings enables follower targeting once. A later explicit OFF is preserved. This migration does not enable automatic outfits.
Keep .ess and .skse together. To downgrade, restore pre-update user data and saves as well as the previous DLL: older versions do not understand all newer preset, library and appearance data.

## Controls
F8 opens Gallery. Default gamepad alternative: hold Back/View for 0.8 seconds. F9 registers an outfit, F10 headgear, F11 accessories while Gallery is open.
Click a photo to apply it; right-click for equipment management. Gamepad A applies, R3 opens the photo menu, B closes. Bindings can be changed in Options.
Choose Normal outfits or Transmog at the top. The selected button is amber; a subdued header band indicates transmog mode.

## Transmog
Changes the visible armor while preserving real equipment, armor rating and enchantment effects. Player first-person and third-person appearance and follower appearance are supported.
Appearance and mode are stored per actor in the SKSE co-save. Use Restore actual appearance to show actual equipment again.
Armor models must support the actor's race and sex. Shield slot 39 and zero-biped-slot visual overrides are not supported. Missing/incompatible visual pieces may be skipped; an entirely unusable preset leaves the previous look intact.
Another appearance mod controlling the same actor may conflict. Startup verifies the required callsites and supported hook chains. If an unsafe/unrecognized site is found, transmog is disabled with a reason available on the transmog control. Listing a runtime is not a guarantee for every hook combination.

## Followers
Follower targeting is ON by default: aim at a currently following humanoid and open Gallery; look away to target the player. Target followers (N) counts followers with maintained outfits, not all nearby followers; zero does not prevent selecting a new follower.
Fixed outfit maintenance and conditional automatic outfits are separate optional controls. Both start OFF for a new follower. Previously saved follower settings are restored from the co-save. Conditional automatic outfits support up to two registered followers.
Other follower managers can compete with outfit maintenance. Restore standard outfit returns the follower to the current default outfit definition; it does not restore an entire inventory.

## Remove and restore equipment
Remove equipment is at the right of the registration/camera toolbar. A compact minus button is used if space is limited.
In its dialog, CHECK to remove one item immediately; UNCHECK to restore that item. Removed rows remain until the dialog is closed or refreshed. Closing keeps the current equipment state. Refresh rebuilds the list and ends the ability to restore items removed from the previous list.
Normal mode lists worn armor/clothing. Transmog lists applied visual overrides; actual equipment outside the controlled scope is not listed.
Removal can expose underwear or nudity. Actions occur immediately. Restoring does not generate items; if the original item or compatible state cannot be identified, refresh is requested.
Head/accessory photo repeat-click toggles are also available when all registered pieces are present. Full-outfit repeat-click removal is OFF by default; enable it explicitly in Options if wanted. Registered head/accessory sets may contain body clothing, so their contents determine what is removed.
Automatic outfit application equips outfits; it does not use repeat-click removal.

## Organize and automatic outfits
Organize gallery supports multiple selection, order changes, Trash and batch registration to existing category tabs. Drag selected photos to a destination tab, or use the destination control. Already-registered counts are shown. Remove from this tab removes category membership without deleting the preset or photo. Changes are committed with Save and close; Cancel discards the draft.
Automatic outfits are enabled separately after assigning photos to conditions. Defaults for new settings: normal delay 0 seconds, combat minimum interval 2 seconds, sneak delay 1 second. Existing explicit settings are preserved. Crafting is still experimental and may skip an attempt if its preparation deadline is missed.
Item counts default OFF. UI zoom, portrait layout, category filtering, camera presets, preset editing and optional independent preview remain available.

## Data, diagnostics and license
Japanese detailed manual: README.ja.md. Release summary: RELEASE-NOTES.md. Development history is retained in the source archive.
Logs are written to the SKSE log directory as OutfitGallery.log and may include preset names and paths. Detailed logging is optional and resets on restart.
Known limitations include paused character animations with Fluid Motion, text blur in some upscaling configurations, and untested rendering/HDR combinations. The independent preview shares the game's rendered image.
GPL-3.0. See LICENSE, THIRD_PARTY.md and LICENSES. Publish the matching source archive with this release. Authored with substantial AI assistance.
