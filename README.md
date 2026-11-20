# Outfit Gallery 2.2.9

## Changes in 2.2.9 (since 2.2.8)

- Organizer tabs now support both browsing and registration: click to browse, drop photos to register. Clicking never registers selected photos.
- Match the main gallery tab order, add headgear/accessory icons, and open the trash list from the upper trash icon.
- Clear processed selections after dropping onto a category; incompatible photos remain selected.
- Add category membership controls to photo context menus. Make menus wider and open near the cursor, or near the target photo for controller use.
- Add Wear set to random try-on: equip the displayed trial without saving it as a gallery entry.
- Mention the right-view double-click gesture in the photo pause/resume tooltip.
- Expand automatic outfit selection to a maximum of six columns.
- Add player-only Cold region and Cold exposure automatic outfit assignments. Missing optional cold information no longer blocks ordinary automatic switching.

### Cold outfit rules

Cold region uses the official Survival Mode freezing-region classification, regardless of time or weather. Cold exposure calculates environmental cold using region, night, rain and snow. Neither requires Survival Mode to be ON; both require its data plugin, ccQDRSSE001-SurvivalMode.esl. This is not a full simulation of accumulated player cold or clothing warmth.

Assign either cold slot: assigning one clears the other. Combat, sleeping, swimming, sneaking and dedicated sheltered locations take priority; outdoors the selected cold rule takes priority over ordinary snow, rain and night outfits. Leave both empty to retain ordinary rules.

Explanations appear only at the bottom inside unassigned cards and disappear when a photo is assigned. The Cold exposure tooltip provides additional detail.

### Organizer and try-on

Organizer changes are committed with Save and close; Cancel discards them. Built-in headgear/accessory tabs are for browsing, not converting a full outfit into another set type.

Wear set changes actual equipment. It uses the existing missing-item, property reproduction and generated-item cleanup settings and protections.

### Updating and ongoing investigation

Exit Skyrim and install with priority over older/trial DLLs. Preserve photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. No new game or outfit re-registration is required. Existing requirements and supported runtimes are unchanged. Hair/FSMP integration from 2.2.8 retains its documented scope.

A report of generated items remaining in inventory on 2.2.4 is still under investigation. The supplied diagnostic portion shows already-owned items and zero tracked cleanup candidates; it does not include detailed generation evidence. This update does not claim to resolve that report.

---

# Outfit Gallery 2.2.8

## Changes in 2.2.8 (since 2.2.5)

- Normal outfit changes can now skip entries whose source mod/base equipment is missing and apply the remaining equipment. Skipped items are listed, and saved outfit data is kept. If no equipment is available, the current actual equipment is left unchanged.
- If a saved custom enchantment requires an unavailable mod or record, equip the item without that saved custom enchantment while retaining its saved smithing improvement. Apply the rest of the outfit and report the affected item/reference.
- Include missing plugin names or reference IDs in enchantment errors, with localized notices.
- Reopening the gallery now starts with Options collapsed, even if it was open when the gallery was closed. Setting values are preserved.
- Replace the main gallery's circular favorite badge background with a star outline and subtle shadow.
- Add favorite badges to organizer cards and expanded variations. The organizer uses a circular background for visibility and positions badges slightly inward from the right edge. Unsaved membership changes are reflected immediately.
- Double-click the right-hand viewing area to pause or resume time for photos, with independent preview either ON or OFF. The existing left-hand playback button remains available.

- Fix a transmog hair-disappearance case caused by secondary rendering markers spanning slots31/41. Confirmed in both directions between actual Painter Hair and Yuna appearances.
- On the specifically supported FSMP build, keep native-hair physics active while a transmog look shows native hair over an actually equipped wig. Actual equipment, enchantments, armor rating and global FSMP settings are preserved. See the additional integration scope below.

### Missing equipment

Filtering only affects the current outfit application; saved presets are not rewritten. Restoring the source mod makes its equipment available again without re-registration. Remaining equipment follows the saved smithing/enchantment reproduction settings. Verification uses available entries, allowing the existing generated-item cleanup checks to proceed after a successful match.

This applies to normal equipment changes. Transmog and equipment-editing reference checks remain unchanged. If base equipment still exists but a saved custom enchantment reference is unavailable, omit that entire saved custom enchantment for the current application. Individual effects are not selectively discarded. Keep the saved recipe so restoring the provider mod allows reproduction without re-registration. Identified owned items and enchantments built into base armor are not stripped. Changed record types, incompatible slot layouts, malformed recipes and ambiguous instance identity continue to stop application. Missing-item generation still requires Add missing base items.

### Double-click controls

Left-double-click without dragging inside the right-hand character viewing area. The action is confirmed on the second release. Dragging, releasing outside the area, or using the right button cancels the gesture. Existing left-drag positioning, right-drag orbit/angle and mouse-wheel distance controls remain available.

Time paused by Outfit Gallery resumes when the gallery closes. Time already paused by another feature is not released. No new permanent right-hand playback button is included in this release.

### Transmog hair and FSMP integration scope

Wig replacement uses confirmed worn-item/addon ownership, not equipment or mod names. This does not promise compatibility with every wig, mixed body/head pieces or ambiguous shared ownership.

The additional native-hair physics bridge currently supports **Skyrim1.6.1170 with the measured FSMP4.1.1 AE AVX binary, player only**. Other4.1.1 CPU variants (SSE2, AVX2 and AVX-512) are not included. It activates automatically only when the image identity and measured function code match. Unknown or modified builds disable only this additional bridge. Existing normal-equipment/transmog runtime support is unchanged, and FSMP remains optional.

On that supported build, native hair can continue simulating with FSMP's Disable hair physics when there's a wig option ON while the transmog appearance shows native hair. Wig appearances, clearing transmog and a full game restart/save reload were confirmed by the user. No global FSMP configuration is rewritten. Outside the bridge's scope, turning that FSMP option OFF is an available workaround, at the possible cost of continuing physics on native hair hidden by wigs.

Actual head equipment is retained with its stats. Native hair is not generated as a separate wig. No global physics reset or extra equip/unequip cycle is added; the bridge scopes its headpart activity adjustment to the existing supported physics update.

### Updating

Exit Skyrim and install with priority over older/trial DLLs. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. No new game or outfit re-registration is required. Requirements and the supported runtime list are unchanged.

---

# Outfit Gallery 2.2.5

## Changes in 2.2.5

- Save custom armor enchantments with outfits and reproduce them together with smithing improvements when the original items are not in inventory. Supports the player and followers, including reproduction in a new game.
- Support copied enchantments with multiple effects, within the supported format and a maximum of 32 effects.
- Show [Enchanted] beside enchanted equipment in equipment management.
- Add Reproduce smithing and enchantments. OFF generates missing items with base properties while preserving owned-item properties and saved recipes. Transmog is unaffected.
- Offer a reproduction choice when first opening the gallery in a save without a recorded choice. Add Skip first-use confirmation to both that screen and Options.
- Allow reproduction when the engine changes an enchantment's gold-value override but its effects, smithing and other required properties still match. The original and reproduced items may have different prices.

### Saving and reproducing equipment

Wear the improved/custom-enchanted armor and register an outfit, or update an existing outfit through equipment management's add/replace-from-current-equipment action. Existing outfits do not acquire enchantment recipes automatically. Supported recipes already saved by the enchantment trials carry over.

To reproduce missing equipment with its saved properties, enable Add missing base items, Reproduce smithing and enchantments, and Prefer registered enchanted items. Identified originals and tracked or protected items associated with that recipe are reused when available; otherwise, a new item is generated. This does not retrieve the original item from a chest.

Turning reproduction OFF only changes missing-item generation. Built-in enchantments on the base armor remain. Owned originals and identified protected items retain their smithing and enchantments. Saved recipes remain intact and can be used again by turning reproduction ON, without re-registration.

The first-use answer and later reproduction changes are recorded for the active save when you save the game. Skip first-use confirmation is global: unanswered saves use the last configured reproduction default when skipping is enabled. Saves with an existing answer retain their own choice. Loading an older unanswered save may show the prompt again.

### Modified items and supported scope

Items further improved or enchanted after generation are protected from cleanup and reused with their originating outfit while identifiable. Their new properties do not automatically update the outfit recipe. To reproduce those properties in a new game, wear the modified item and update the outfit. Cleanup only claims tracked generated instances, never all items sharing the same base type.

Supports compatible constant-effect/self-target enchantments on armor, clothing and jewelry, up to 32 effects, subject to reference, numeric and format validation. Per-effect conditions and temporary enchantments are unsupported. The original armor/effect-providing mods are still required. Single/dual effects, copied multi-effect enchantments, and smithing combinations were tested in game. Nonzero effect area/duration remain unverified in game.

Custom display names and arbitrary item data supplied by other mods are not fully preserved or reproduced. Mods that only rename an item differ from mods that replace its instance; compatibility needs individual verification.

### Updating

Exit Skyrim and install with priority over older/trial DLLs. No new game is required. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. Back up before updating: editing/saving an outfit in an older version may discard the new enchantment data. Requirements and the supported runtime list are unchanged.

---

Previous release history and existing feature documentation follow.

# Outfit Gallery 2.2.4

## Changes in 2.2.4

- Save and reproduce smithing improvements on armor for the player and followers, including when the registered item is no longer owned.
- Improve generated-item tracking and cleanup, including items that already receive an instance ID during creation. Preserve items further improved or custom-enchanted after generation and reuse them with their original outfit.
- Show the saved smithing quality, such as Fine, in equipment management using the game's localized naming.
- Make click-to-expand the default for outfit variations in the normal gallery. Hover expansion remains optional. Click-opened strips close after the pointer leaves both the source and strip for about 0.25 seconds.
- Up/down while a controller variation strip is open now only closes it and returns focus to the original group. Release and press again to navigate normally.
- Add a gallery background transparency slider and reset. Place UI scale/reset and independent-preview controls on compact rows, with preview help in tooltips.
- Group organizer registration targets by normal outfits then accessories, preserving order within each type and existing memberships. The view selector follows the main gallery's normal/head/accessories ordering.

### Smithing: what is saved

Wear the improved armor and register the outfit. Re-register an older outfit if you want to add its smithing recipe; existing ordinary outfits still work unchanged. Recipes saved by the smithing trials carry over.

Applying a recipe prefers its identified original item, then a tracked or retained reproduction associated with that saved recipe. If neither is available, enabling Add missing base items permits a new item with the saved smithing value. With generation disabled, application stops with a notice. Existing ordinary items of the same base type are not modified or deleted to reproduce a recipe.

Saved smithing values can carry across a new game. Further improvements to an owned item do not automatically update the saved recipe; equipment management displays the saved value. Legacy outfits without smithing metadata keep the previous behavior.

In 2.2.4, custom enchantments, custom names and poison were not recreated. See the 2.2.5 section above for current enchantment support. The existing enchantment-preference option uses owned enchanted items. Items further improved or custom-enchanted after generation are protected from cleanup. Unidentified items are never mass-deleted by base type, and previously untracked leftovers are not retroactively adopted for cleanup.

### Updating to 2.2.4

Exit Skyrim and install this release with priority over earlier trial DLLs. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. Back up the existing data before updating. No new game is required. Older versions may not retain the new smithing metadata. Runtime requirements and the supported executable list are unchanged.


## 2.2.2

- Fixed missing ears or an invisible head when real headgear is equipped during transmog.
- Added **Move to another tab** to the organizer's right-click menu, plus **Alt + drag** onto a destination tab. Only the current tab's membership is removed; other tabs are kept. Ordinary dragging still adds a registration, and Shift-click still selects a range.
- Added a **Sleeping** automatic outfit slot for the player and registered automatic-outfit followers. An empty slot leaves sleep switching inactive. After waking, outfits follow the current automatic conditions.

### Moving between tabs

Select outfits in the organizer, right-click and choose Move to another tab, then choose a destination. Alternatively, hold Alt while dropping the selection onto a registration tab. Transfers apply only when you Save and close; Cancel discards the draft. Moves require a category tab as the source and another tab of the same type as the destination. All, built-in Head/Accessories and Trash are not move sources. Moving to a tab that already contains the selection still removes its source membership. Photos, outfit contents, other memberships and photo order are preserved.

### Sleepwear

Enable automatic outfits and assign a full outfit to Sleeping. The selected equip/transmog mode is respected. Game sleep states and the player's sleep-time menu are detected; simply waiting, sitting or reaching night does not trigger sleepwear. After waking, ordinary condition evaluation resumes, with the usual delays and equipment queue. Assign Default or an appropriate location outfit too; if no outfit is assigned to the waking condition, the current outfit can remain.

Avoid using another sleepwear feature at the same time. The short reminder is shown only while Sleeping is unassigned. Mods that use custom animations without the game's sleep state may not trigger this feature; compatibility with individual sleep mods is not guaranteed. Registered automatic followers are supported; this does not distribute sleepwear to every NPC.


## 2.2.1

- Improved random try-on matching for part names and A/B/C variants, reducing omitted matching pieces.
- L1/R1 tab switching now moves focus to the outfit list. Left/right navigation while focused on the tabs is unchanged.
- Added a photo pause/resume button beside camera preset 7. Blue indicates the pause action; orange indicates an active pause. Camera and gallery controls remain available. Closing the gallery releases pauses started by this feature.
- Added failure diagnostics to detailed logging for transmog startup validation. Hook acceptance rules are unchanged.

Known compatibility report: Transmog has been reported unavailable with ApparelPreview (Nexus 185334). Disabling ApparelPreview restored transmog in the reported setup. Compatibility remains unverified; no compatibility bypass is included.


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

In Organize gallery, select matching variations and choose Group variations (also available on right-click). The leading photo represents the group. Save and close commits organizer changes; Cancel discards them. A representative has a +count badge. Click to expand by default; hover expansion can be enabled in Options. Click-opened strips close after leaving both source and strip for about 0.25 seconds. Controller users can open with A, navigate left/right, and close with B or up/down; up/down only cancels the strip until released and pressed again. Right-click actions keep the strip visible. Overflow uses the available gallery width and horizontal scrolling.

In the organizer, the representative selects the whole visible group; individual expanded photos select independently. Detach removes only selected members. Grab the exposed top of the original representative to drag the group while expanded. Grouping is manual, for 2–100 compatible presets of the same type/slot scope. Search and tabs show only matching members. Grouping does not delete the original photos or merge equipment data.

Blank full-outfit registration names prefer the body item; other registrations use the first named registered item. Remove all / Restore all acts on the current removal-window list with verified sequential operations. Closing keeps the current state; refreshing resets its restoration list. Removing clothes can expose skin.

## Updating / 更新
Exit Skyrim and install the complete 2.2.4 archive. Disable earlier trial DLL overrides, including next-update-test1, transmog diagnostic builds, variations-test12, accessory-tabs and equipment-list trials. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. Back up Library.json before updating; saving with older releases can discard new tab/group metadata. No new game or preset re-registration is required. Existing external translations still work; untranslated new keys fall back to English.

Skyrimを終了し、完全版2.2.4を導入してください。next-update-test1/2/3・headgear-test1・重ね着診断版・test12などの試作DLL・旧差し替えパッチを無効にし、写真・プリセット・Library.json・設定・カスタムINI・対の.ess/.skseは保持してください。更新前にLibrary.jsonをバックアップしてください。旧版で保存すると追加タブ・差分の情報が失われる場合があります。ニューゲーム・再登録は不要です。外部翻訳の新規未翻訳項目は英語になります。

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
Keep generated Data/SKSE/Plugins/OutfitGallery data: photos, presets and JSON settings. Preserve a customized OutfitGallery.ini. With MO2, check the existing mod folder, Overwrite and any output mod before replacing files. Disable older trial DLL overrides and the standalone 1.7.104 Transmog Fix so the 2.2.4 DLL takes priority. Hotkeys.json takes priority over INI hotkeys. A new game or preset re-registration is not required.
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
