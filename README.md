# Outfit Gallery 2.2.0

## Changes in 2.2.0
- Equipment plugins: include accessory-only plugins; consolidate entries with identical acquisition sets while retaining patches with different equipment. Hidden names remain searchable aliases.
- Create accessory-only tabs, placed beside the built-in Accessories tab. R2 cycles through accessory tabs. Newly registered accessories still enter the built-in tab first.
- Group outfit variations under a representative photo. Hover to expand, choose a variation, and scroll horizontally with the mouse wheel or controller.
- Organize groups in the organizer: group selected presets, detach only selected members, and drag the representative to move the entire group. Selection clears after grouping or detaching.
- Generate a name from registered equipment when the registration name is blank. Explicit names and existing presets are preserved.
- Suspend mouse hover/cursor during controller use; resume on mouse input, preventing stationary-cursor tooltips and accidental variation expansion.
- Add Remove all equipment / Restore all equipment to the equipment-removal window.

## Variation groups and accessory tabs
Create an accessory tab in Options using Create tab. It accepts accessory presets, not full outfits; grouping does not change which slots a preset equips. New accessory registrations remain in the built-in Accessories tab; assign them to custom tabs afterward.

In Organize gallery, select matching variations and choose Group variations (also available on right-click). The leading photo represents the group. Save and close commits organizer changes; Cancel discards them. A representative has a +count badge. Hover for about 0.25 seconds to expand; leaving the source and strip for about 0.1 seconds closes it. Controller users can open with A, navigate left/right, and close with B or up/down. Right-click actions keep the strip visible. Overflow uses the available gallery width and horizontal scrolling.

In the organizer, the representative selects the whole visible group; individual expanded photos select independently. Detach removes only selected members. Grab the exposed top of the original representative to drag the group while expanded. Grouping is manual, for 2–100 compatible presets of the same type/slot scope. Search and tabs show only matching members. Grouping does not delete the original photos or merge equipment data.

Blank full-outfit registration names prefer the body item; other registrations use the first named registered item. Remove all / Restore all acts on the current removal-window list with verified sequential operations. Closing keeps the current state; refreshing resets its restoration list. Removing clothes can expose skin.

## Updating / 更新
Exit Skyrim and install the complete 2.2.0 archive. Disable earlier trial DLL overrides, including variations-test12, accessory-tabs and equipment-list trials. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. Back up Library.json before updating; saving with older releases can discard new tab/group metadata. No new game or preset re-registration is required. Existing external translations still work; untranslated new keys fall back to English.

Skyrimを終了し、完全版2.2.0を導入してください。test12などの試作DLL・旧差し替えパッチを無効にし、写真・プリセット・Library.json・設定・カスタムINI・対の.ess/.skseは保持してください。更新前にLibrary.jsonをバックアップしてください。旧版で保存すると追加タブ・差分の情報が失われる場合があります。ニューゲーム・再登録は不要です。外部翻訳の新規未翻訳項目は英語になります。

Supported runtimes and the optional upscaler compatibility default (OFF) are unchanged. This release does not claim new testing on every supported runtime. GOG 1.6.1179 remains untested.


A photo-based outfit gallery with normal equipment changes and appearance-only transmog for the player and followers.

## Changes in 2.1.0
- Added Equipment & random try-on for the player: discover compatible armor from installed plugins without an external item menu.
- Filter plugin names (including UBE patch names), select a plugin from the newest-file-first list, and draw a matching outfit.
- Reroll within the displayed body equipment's plugin, including outfits applied from saved presets.
- Customize individual parts, swap slot conflicts, and remove/re-add parts immediately while keeping the character preview visible.
- Acquire one of each playable armor item defined in the selected plugin, after reviewing a confirmation list.
- Show/hide the equipment tools with the top-bar Equipment button. The choice is saved; default ON.
- Retains the normal outfit, follower, transmog and compatibility features of 2.0.1.

## Equipment acquisition and random try-on
These tools are player-only. Open Equipment & random try-on below the registration buttons. Enter part of a plugin filename or select an equipment plugin. The list uses file creation timestamps visible through the game's Data directory; it approximates recent installation and is not an MO2 installation-history database. A winning UBE patch name can match a base plugin's equipment. This is a filename filter, not an automatic body-type guarantee.

Draw and try on changes appearance only, using the transmog system; actual equipment and stats stay unchanged. Reroll uses the currently displayed body item's source plugin, irrespective of the filter. End try-on or close Gallery to restore the appearance from before the trial. Save try-on preset to keep a combination and its photo for later. Closing the customization window keeps the trial; its reset button restores the combination from when that window was opened.

Matching uses outfit records, shared names, color/number variants and non-overlapping equipment slots. Fill empty slots relaxes the name matching within the same plugin. Matching is heuristic: incomplete sets, unintended combinations, clipping or exposed skin are possible. Customize the parts before saving. Items without a compatible race/sex model are not offered for try-on; body equipment must use slot 32. The number and order of random results are not guaranteed, and reroll can repeat when alternatives are limited.

Acquire plugin equipment is separate from try-on. The confirmation list contains named, playable armor defined in or currently overridden by the selected plugin, including color variants and already-owned items; each is added once per confirmation. Weapons are excluded. Acquisition is not race/body filtered and a base-plugin selection does not automatically acquire all items newly defined in a separate patch. It changes the real player inventory and is not undone by End try-on. Closing Gallery during a large acquisition can leave the items already processed in inventory.

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
Keep generated Data/SKSE/Plugins/OutfitGallery data: photos, presets and JSON settings. Preserve a customized OutfitGallery.ini. With MO2, check the existing mod folder, Overwrite and any output mod before replacing files. Disable older trial DLL overrides and the standalone 1.7.104 Transmog Fix so the 2.2.0 DLL takes priority. Hotkeys.json takes priority over INI hotkeys. A new game or preset re-registration is not required.
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
