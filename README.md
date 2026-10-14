## Changes in 1.0.15

- Added experimental automatic outfits for up to two followers, with individual assignments and timing settings.
- Added outfit checks after travel, including early recovery when continuing the same outfit. Ready player changes take priority over waiting follower changes.
- Added bulk Move to Trash from selected photos or the gallery organizer toolbar, and bulk restoration from Trash.
- Fixed the gallery selection border alignment.
- L2/R2 category switching now focuses the photos for immediate navigation.
- Added Japanese translations for Empty Trash and Delete permanently.
- Added protection against the held opening button immediately closing the gallery.

### Automatic follower outfits (experimental)

Enable follower selection, aim at an accompanying follower, open OG and switch to automatic outfits. Check the target name, assign full outfits and enable automation.

- Register up to two followers. Disabled configurations still occupy a slot; remove one under the registered-followers list to free it.
- Assignments and enabled state are stored when saving the game. Save after editing; loading restores the configuration in that save.
- Enabling automation disables fixed outfit maintenance for that follower. Turning automation off does not automatically re-enable maintenance.
- Conditions are evaluated for the follower: location, weather, time, sneaking, swimming and combat. Clothing/armor changes preserve weapons and spells. Missing inventory items are generated.
- Crafting outfits and the automation toggle hotkey remain player-only.
- Changes are processed sequentially. Travel recovery is bounded; it does not continuously override another mod's outfit changes.
- Enable detailed diagnostic logging in Options when investigating problems. Travel/equipment diagnostics are included. Logging resets to OFF on game restart; diagnostic inventory scans do not run while OFF.

# Outfit Gallery 1.0.15

A photo-based SKSE outfit gallery for full outfits, head equipment and accessories. Weapons and ammo are preserved. Includes conditional automatic outfits, follower selection/maintenance, preset editing and an optional independent preview.

Japanese manual: [README.ja.md](README.ja.md). New features: [FEATURES.ja.md](FEATURES.ja.md).

## Requirements

Install SKSE matching your Skyrim executable, Address Library, and SKSE Menu Framework 3 with its dependencies. This plugin uses the framework's event-priority and input-event API. Disable the framework's Blur Background option for a clear subject. Japanese display requires a suitable framework font. SmoothCam and True Directional Movement are optional. Outfit mods, fonts and third-party plugin DLLs are not bundled.

Accepted executables: Steam 1.5.97, 1.6.353, 1.6.640, 1.6.1130, 1.6.1170, 1.7.104; GOG 1.6.1179. Development gameplay testing has been reported on 1.5.97 and mainly 1.6.1170; a compatibility build was user-tested on 1.7.104. This is not a fresh test of every feature on every executable. GOG and the other listed targets remain untested. VR is not supported. For Steam 1.7.104 use matching SKSE, Address Library and Menu Framework versions.

## Install or update

Close Skyrim and install this complete archive with your mod manager. No ESP or Papyrus scripts are required. Do not let an older trial DLL override this release. Keep generated Data/SKSE/Plugins/OutfitGallery data, including photos, presets and settings, and any customized OutfitGallery.ini. Before replacing an MO2 mod, preserve generated data inside that mod; also check Overwrite and your output mod. Saved JSON settings are not distributed or reset. Hotkeys.json takes priority over INI input settings.

New accessory presets use schema 4, including zero-slot armor. Older versions cannot read these new presets and may discard newer library fields. Restore pre-update data as well as the old DLL when downgrading. Keep .ess and .skse save files together.

## Basic controls

F8 opens the gallery; gamepad Back/View held for 0.8 seconds is the default alternative. F9 registers a full outfit, F10 head equipment, F11 accessories while the gallery is open. Click a photo to equip it; right-click for management. Gamepad A applies, Y/X/left-stick click register the three modes, right-stick click opens the photo menu and B closes. Bindings can be changed in Options. English/Japanese are built in; the translations folder contains an external translation template, not an active override.

## Automatic outfits

Open Auto outfits, assign photos to conditions, then enable automatic switching. Conditions cover locations (including indoor shops), weather/night, swimming, sneaking, combat styles and crafting stations. Empty specialized combat slots fall back to the combat base. Location detection depends on game records and may differ in mod-added locations. Combat and crafting remain experimental. Crafting makes one pre-menu attempt and may skip if the timing deadline has passed. After leaving a station, normal automatic condition handling resumes.

The dedicated shortcut in the automatic-outfit options toggles automatic switching itself, not its panel. It is unbound by default, supports keyboard modifiers and gamepad hold, and resumes a paused system. Delay settings and OStim scene suspension are available.

## Equipment management

Full outfits use individual equipment selection; head/accessory presets use exchange slots in the management screen. Zero-slot armor can be selected individually. Accessories capture worn armor too; clear the selection and select just the desired ring/item when needed. An empty saved selection does nothing. Unrelated equipment is preserved for partial changes. Full outfit presets can be edited from currently worn armor; review before saving. One initial original is kept for restoration, not a growing edit history. Windows resize and scroll; outside clicks do not close them.

## Data and limitations

Missing base items can be generated if enabled; custom enchantments, tempering and custom names are not cloned into new items. Player enchanted-item preference uses owned registered items. Save the game after registering them. Removed mods or changed form/slot definitions can invalidate a preset. Missing-set checking reports unavailable records, not missing inventory items.

Follower selection targets loaded humanoid followers, including the hired Sofia fallback. The header count refers to maintained follower outfits; turning selection off does not cancel maintenance. Other follower managers may compete with outfit control. Independent preview shares the rendered game image; it does not create a second camera or clone the actor. Rendering-mod and HDR combinations are not comprehensively tested.

Diagnostics are in OutfitGallery.log in the SKSE log directory. Detailed logging is optional and resets on restart. Logs can include preset names and local paths.

## License and provenance

GPL-3.0; see LICENSE, THIRD_PARTY.md and LICENSES. The matching source archive is provided separately. This project was authored with substantial AI assistance. See VALIDATION.md for the tested scope.

## Gallery display and camera preset names (1.0.15)

Normal thumbnails support 2–6 columns; portrait thumbnails support 3–10. Start/Menu cycles the gallery columns only when it is not assigned to opening Gallery, toggling automatic outfits or capturing outfits in OG. Existing assignments take priority.

Options → Compact display hides the preset-name and search fields and moves Organize gallery next to Normal/Portrait. It defaults to OFF and is remembered. Enabling it clears the search filter; registration uses automatic names. Turn it OFF to enter names or search. Photo names and item counts retain their separate visibility settings. Follower maintenance controls share the row only when the translated controls fit.

Open Camera Settings using the camera icon, select a preset slot and save its name. Numbered buttons remain unchanged; hover to see the name. Saving an empty name restores the default tooltip. Renaming does not change the camera position or capture it. Names persist across restarts.

Equipment to wear now shows all biped slot numbers before each item name. This is a display change; selection behavior is unchanged.
