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

# Outfit Gallery 2.2.7

## Changes in 2.2.7 (since 2.2.5)

- Normal outfit changes can now skip entries whose source mod/base equipment is missing and apply the remaining equipment. Skipped items are listed, and saved outfit data is kept. If no equipment is available, the current actual equipment is left unchanged.
- If a saved custom enchantment requires an unavailable mod or record, equip the item without that saved custom enchantment while retaining its saved smithing improvement. Apply the rest of the outfit and report the affected item/reference.
- Include missing plugin names or reference IDs in enchantment errors, with localized notices.
- Reopening the gallery now starts with Options collapsed, even if it was open when the gallery was closed. Setting values are preserved.
- Replace the main gallery's circular favorite badge background with a star outline and subtle shadow.
- Add favorite badges to organizer cards and expanded variations. The organizer uses a circular background for visibility and positions badges slightly inward from the right edge. Unsaved membership changes are reflected immediately.
- Double-click the right-hand viewing area to pause or resume time for photos, with independent preview either ON or OFF. The existing left-hand playback button remains available.

### Missing equipment

Filtering only affects the current outfit application; saved presets are not rewritten. Restoring the source mod makes its equipment available again without re-registration. Remaining equipment follows the saved smithing/enchantment reproduction settings. Verification uses available entries, allowing the existing generated-item cleanup checks to proceed after a successful match.

This applies to normal equipment changes. Transmog and equipment-editing reference checks remain unchanged. If base equipment still exists but a saved custom enchantment reference is unavailable, omit that entire saved custom enchantment for the current application. Individual effects are not selectively discarded. Keep the saved recipe so restoring the provider mod allows reproduction without re-registration. Identified owned items and enchantments built into base armor are not stripped. Changed record types, incompatible slot layouts, malformed recipes and ambiguous instance identity continue to stop application. Missing-item generation still requires Add missing base items.

### Double-click controls

Left-double-click without dragging inside the right-hand character viewing area. The action is confirmed on the second release. Dragging, releasing outside the area, or using the right button cancels the gesture. Existing left-drag positioning, right-drag orbit/angle and mouse-wheel distance controls remain available.

Time paused by Outfit Gallery resumes when the gallery closes. Time already paused by another feature is not released. No new permanent right-hand playback button is included in this release.

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

# Changelog

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

## 2.2.4 更新内容

- 鍛冶で強化した防具の強化度をセットに保存し、対象品を持っていない場合にも再現できるようにしました。プレイヤーとフォロワーに対応します。
- 生成した装備の識別・回収を改善しました。生成時点ですでに個体番号の付いた装備にも対応し、後から追加強化・自作付呪した生成品は保護して元のセットで再利用します。
- 装備管理の装備名に、保存した強化度に対応する「優良」などの段階を表示します。
- 通常画面の差分展開をクリック方式に変更しました。オプションで従来のマウスオーバー方式も選べます。クリック展開後、元写真と展開枠から約0.25秒離れると閉じます。
- パッドで差分展開中に上下を入力すると、移動せず展開だけ閉じ、元のセットへ戻ります。一度離して再入力すると通常移動します。
- 左側ギャラリー背景のUI透過率スライダーとリセットを追加しました。UI倍率・リセットを横並びにし、独立プレビューの説明をツールチップへ整理しました。
- 整理画面の登録先タブを通常→小物の順にまとめました。表示切替はメイン画面と同じ通常→頭装備→小物の順です。同じ分類内の順序と各タブへの登録内容は維持します。

### 鍛冶強化の保存と再現

強化した防具を実際に装備してセットを登録してください。既存セットに後から強化度を保存する場合は再登録が必要です。試験版で強化度を保存済みのセットは引き継げます。通常の既存セットはそのまま使用できます。

適用時は識別できる登録元の現物を優先し、続いて同じ登録強化度に紐付く生成品・保護済みの生成品を再利用します。見つからない場合は「不足している装備を追加して復元」が有効なら新しく生成して保存した強化度を設定します。無効なら生成せず案内します。同名の未強化所持品を強化したり削除したりする機能ではありません。

保存した鍛冶強化度はニューゲームでも再現できます。後から現物をさらに強化しても、セットの保存値は自動更新しません。装備管理の段階表示も保存値を示します。強化情報のないセットは従来の表示・処理を使います。

2.2.4時点では自作付呪・独自名称・毒の複製は対象外でした。2.2.5の付呪対応は上記を参照してください。自作付呪品を所持している場合の優先使用は従来の付呪優先機能です。生成後に追加強化・自作付呪した品は回収から保護します。識別できない品を同名という理由だけで一括削除することはありません。以前の版で回収対象として記録されなかった残留品を、一律に削除する変更でもありません。

### 更新方法

Skyrimを終了し、旧版や試験版のDLLよりこの公開版が優先されるよう導入してください。写真・プリセット・Library.json・設定・カスタムINI、対になった.ess/.skseを保持してください。通常利用にニューゲームは不要です。更新前のデータを保管してください。旧版では新しい強化情報を保持できない場合があります。対応ランタイムと必須環境は従来どおりです。


## 2.2.2

- Fixed missing ears or an invisible head when real headgear is equipped during transmog.
- Added **Move to another tab** to the organizer's right-click menu, plus **Alt + drag** onto a destination tab. Only the current tab's membership is removed; other tabs are kept. Ordinary dragging still adds a registration, and Shift-click still selects a range.
- Added a **Sleeping** automatic outfit slot for the player and registered automatic-outfit followers. An empty slot leaves sleep switching inactive. After waking, outfits follow the current automatic conditions.

## 2.2.1

- Improved random try-on matching for part names and A/B/C variants, reducing omitted matching pieces.
- L1/R1 tab switching now moves focus to the outfit list. Left/right navigation while focused on the tabs is unchanged.
- Added a photo pause/resume button beside camera preset 7. Blue indicates the pause action; orange indicates an active pause. Camera and gallery controls remain available. Closing the gallery releases pauses started by this feature.
- Added failure diagnostics to detailed logging for transmog startup validation. Hook acceptance rules are unchanged.

Known compatibility report: Transmog has been reported unavailable with ApparelPreview (Nexus 185334). Disabling ApparelPreview restored transmog in the reported setup. Compatibility remains unverified; no compatibility bypass is included.

# Outfit Gallery 2.2.0

## Changes in 2.2.0
- Equipment plugins: include accessory-only plugins; consolidate entries with identical acquisition sets while retaining patches with different equipment. Hidden names remain searchable aliases.
- Create accessory-only tabs, placed beside the built-in Accessories tab. R2 cycles through accessory tabs. Newly registered accessories still enter the built-in tab first.
- Group outfit variations under a representative photo. Hover to expand, choose a variation, and scroll horizontally with the mouse wheel or controller.
- Organize groups in the organizer: group selected presets, detach only selected members, and drag the representative to move the entire group. Selection clears after grouping or detaching.
- Generate a name from registered equipment when the registration name is blank. Explicit names and existing presets are preserved.
- Suspend mouse hover/cursor during controller use; resume on mouse input, preventing stationary-cursor tooltips and accidental variation expansion.
- Add Remove all equipment / Restore all equipment to the equipment-removal window.

## 2.2.0の更新内容
- アクセサリのみの装備プラグインも一覧に表示。取得対象が同じプラグインは重複を整理し、装備内容が異なるパッチは個別に残します。非表示になった名前でも検索できます。
- 小物専用タブを作成できるようにしました。標準の小物タブの右に並び、R2で順に切り替えられます。新しく登録した小物は従来どおり標準の小物タブに入ります。
- 色違いなどの差分を代表写真1枚にまとめ、カーソルを合わせて横方向に展開できます。ホイール・パッドで横スクロールできます。
- 整理画面で差分をまとめる、選んだ差分だけ解除する、代表写真からグループ全体をドラッグして並べ替える操作に対応。まとめ・解除後は選択を自動解除します。
- 登録名が空欄の場合、登録装備から自動命名します。入力した名前と既存のセット名は変更しません。
- パッド操作中はマウス位置による名前表示・差分展開を抑止。マウス入力でマウス操作へ戻ります。
- 装備解除ウインドウに「すべて外す」「すべて戻す」を追加しました。

# Outfit Gallery 2.1.0

## Changes in 2.1.0
- Added Equipment & random try-on for the player: discover compatible armor from installed plugins without an external item menu.
- Filter plugin names (including UBE patch names), select a plugin from the newest-file-first list, and draw a matching outfit.
- Reroll within the displayed body equipment's plugin, including outfits applied from saved presets.
- Customize individual parts, swap slot conflicts, and remove/re-add parts immediately while keeping the character preview visible.
- Acquire one of each playable armor item defined in the selected plugin, after reviewing a confirmation list.
- Show/hide the equipment tools with the top-bar Equipment button. The choice is saved; default ON.
- Retains the normal outfit, follower, transmog and compatibility features of 2.0.1.

## 2.1.0の更新内容
- プレイヤー向け「装備取得・おまかせ試着」を追加。外部のアイテム取得ツールを経由せず装備を探して試着できます。
- プラグイン名の検索・一覧選択、同じプラグイン内の再抽選、パーツの交換・解除・復元、一括取得に対応しました。
- 上部の「装備取得」で欄全体を表示・非表示にできます。初期ONで、選択は次回起動にも引き継ぎます。
- 2.0.1の通常着替え・フォロワー・重ね着・各種互換修正を引き継いでいます。


# Outfit Gallery 2.0.1

## English
- Added Upscaler compatibility mode, OFF by default. Enable it in Options if upscaling makes Gallery text blurry or enlarged, then close and reopen Gallery. The choice is saved across game restarts.
- In compatibility mode, added HUD widgets that respond to Cursor Menu can hide while Gallery is open and return afterward. The native game cursor is hidden during Gallery to prevent overlapping pointers.
- Integrated the Steam 1.7.104 Transmog Fix. Verified fallback call positions can be used when normal call-target discovery cannot locate the required hooks.
- Fixed Gallery entry when another mod already owns TDM's headtracking-disable control. This resolves the reported first-person entry failure with ImprovedCameraSE 2.0.0.2231. External ownership is respected.

## 日本語
- 「アップスケール互換モード」を追加しました。初期値はOFFです。アップスケール使用時に文字がぼやける、または大きくなる場合はONにしてギャラリーを開き直してください。選択はゲーム再起動後も維持されます。
- 互換モードでは、Cursor Menuに反応する追加HUDもギャラリーの開閉に連動して非表示・再表示されます。ギャラリー中はゲーム標準カーソルの絵を隠し、二重表示を防ぎます。
- Steam 1.7.104向け重ね着修正を統合しました。通常の呼出先探索で特定できない場合に、確認済みの呼出位置を利用できます。
- 他MODがTDMの頭の追従無効化を既に制御している場合の起動判定を修正しました。ImprovedCameraSE 2.0.0.2231併用時に、一人称から起動できなかった報告に対応しています。

# Outfit Gallery 2.0.0

## English
- Appearance-only transmog for the player and followers, including player first-person view.
- Selected mode and transmog appearance saved per actor in SKSE co-saves.
- Current-equipment dialog: check to remove immediately, uncheck to restore during the same dialog session.
- Repeat-click removal for head/accessory sets, plus optional full-outfit removal, OFF by default.
- Batch category registration and category membership removal in Organize gallery, with draft Save/Cancel.
- Follower targeting ON by default, clearer target/count control. Automatic outfits are enabled separately.
- Refined mode colors, subdued transmog header indication, translated appearance status messages.
- New-settings defaults: item counts OFF, combat minimum switch interval 2 seconds, sneak delay 1 second.
- Startup validation across the accepted runtime list and supported CALL hook chains, with an explanation if transmog is unavailable.

## 日本語
- 実装備と性能を維持する重ね着を追加。プレイヤー一人称・三人称とフォロワーに対応しました。
- 対象別のモードと見た目をSKSEセーブ付随ファイルに保存します。
- 装備一覧からチェックONで即解除、OFFで戻せる小窓を追加しました。
- 頭装備・小物の再クリック解除と、初期OFFの全身セット解除オプションを追加しました。
- 整理画面で写真のタブ一括登録と「このタブから外す」に対応しました。
- フォロワー対象選択を初期ONにし、表示名と人数の説明を改善しました。自動着替えの有効化は別です。
- モード選択色と控えめな帯を整え、成功・リセット時のメッセージを日本語化しました。
- 新規設定のアイテム数表示OFF、戦闘最短切替2秒、スニーク待機1秒へ変更しました。
- 対応ランタイムと既存CALLフックの連鎖を検証し、利用できない場合は理由を表示します。

## Updating / 更新
Close Skyrim; preserve photos, presets, JSON settings, customized INI and paired .ess/.skse saves. Install this complete package and disable older trial DLL overrides. No reset, new game or re-registration is required.
ゲームを終了し、写真・プリセット・設定・カスタムINI・.ess/.skseを保持して更新してください。初期化・ニューゲーム・再登録は不要です。
Legacy follower targeting is migrated ON once; later explicit OFF is preserved. Automatic-outfit enablement is separate.
旧設定のフォロワー対象選択だけ今回一度ONへ移行します。その後のOFFは維持します。自動着替えは有効化しません。

Accepted targets / 対応実装:
Steam 1.5.97 / 1.6.353 / 1.6.640 / 1.6.1130 / 1.6.1170 / 1.7.104
GOG 1.6.1179 (untested / 実機未検証)
This update was user-tested on Steam 1.5.97 and mainly 1.6.1170. Other targets have implementation coverage but have not been gameplay-tested for this update. See VALIDATION.md.
今回の実機確認はSteam 1.5.97と主に1.6.1170です。他の対象は対応実装済み・今回の機能の実機検証なしです。

GPL-3.0. Matching source and third-party notices accompany the release. Authored with substantial AI assistance.
## Changes in 1.0.17

- Automatic outfit change delay now defaults to 0 seconds. Existing saved delays are preserved; sneak delay and combat switching intervals are unchanged.
- Added adjustable UI size (75–200%) with automatic scaling based on the available screen size.
- Improved context-menu positioning and size limits to keep controls within the screen.
- Added a guarded gallery-opening fallback when Fluid Motion - Animation is loaded and TDM directional movement is already disabled by another mod.
- Unified combat opening permission for player and follower targets.
- Combat and free-camera opening now default to ON for new settings. Saved ON/OFF preferences and all other opening restrictions are preserved.

Known limitations: Character animations may pause while the gallery is open with Fluid Motion. Blurry text with upscalers remains under investigation; this update does not change the UI rendering stage or font atlas.

## 1.0.17 の更新内容

- 自動着替えの待機時間を初期値0秒に変更。保存済みの待機時間は維持し、隠密用の待機時間・戦闘時の切り替え間隔は変更しません。
- UI倍率の調整（75～200%）を追加。画面サイズに応じた自動拡縮に対応しました。
- 右クリックメニューの位置・サイズを調整し、操作項目の画面外へのはみ出しを抑えました。
- Fluid Motion - Animationを検知し、TDMの方向移動が他MODによって停止されている対応可能な条件では、ギャラリー起動を継続するようにしました。
- 戦闘中の起動許可をプレイヤー・フォロワー対象で共通化しました。
- 戦闘中・フリーカメラ中の起動を初期値ONに変更しました。保存済みのON/OFF設定と、それ以外の起動制限は維持します。

既知の事項：Fluid Motion併用時、ギャラリー表示中にキャラクターの動きが停止する場合があります。アップスケール環境での文字のぼやけは調査中です。本更新はUIの描画段階やフォントアトラスを変更するものではありません。

## Changes in 1.0.16

- Added an optional setting to remove tracked, generated equipment even when another mod changes its name. Existing-item, favorite, enchantment, tempering and equipped-item protections remain in place.
- Clarified the two cleanup options. Try renamed-item cleanup first; enable additional cleanup compatibility only if generated items still remain.
- Added **Retake photo** to the photo context menu. Preview the new photo in the gallery, then confirm, retake or cancel. Equipment, preset names and automatic outfit assignments remain unchanged.
- The retake screen includes camera presets, column and Normal/Portrait controls, with a compact layout and an unsaved-photo notice.

## 1.0.16 の更新内容

- 他のMODによって名前が変わった生成装備も回収する設定を追加。OutfitGalleryが生成・追跡している個体だけが対象で、元から持っていた物、お気に入り、後から付呪・強化した物、装備中の物は引き続き保護します。
- 2つの回収設定の説明を整理。まず名前変更の設定だけを試し、それでも残る場合に既存の「装備クリーンアップ互換処理」を試してください。
- 写真の右クリック／R3メニューに「写真を撮り直す」を追加。一覧で仮画像を確認し、確定・撮り直し・キャンセルできます。装備内容・名前・自動着替えの割り当ては変わりません。
- 撮り直し画面にカメラプリセット、列数、通常／縦長の切り替えを配置。未確定の案内を撮影ボタンの下に表示します。

## Changes in 1.0.15

- Added camera preset names: edit them in Camera Settings and hover over numbered buttons to see them.
- Added biped slot numbers to the equipment selection list.
- Expanded the normal gallery to six columns. Start/Menu cycles columns when not assigned to another OG action.
- Added optional Compact display to hide the registration-name and search fields. Follower controls share a row when space permits.
- Added experimental automatic outfits for up to two followers, with individual assignments and timing settings.
- Added outfit checks after travel, including early recovery when continuing the same outfit. Ready player changes take priority over waiting follower changes.
- Added bulk Move to Trash from selected photos or the gallery organizer toolbar, and bulk restoration from Trash.
- Fixed the gallery selection border alignment.
- L2/R2 category switching now focuses the photos for immediate navigation.
- Added Japanese translations for Empty Trash and Delete permanently.
- Added protection against the held opening button immediately closing the gallery.

## 1.0.15 の更新内容

- カメラプリセットの名前を設定可能に。カメラ設定内で編集し、数字ボタンにマウスを重ねると表示します。
- 装備管理の「着替えに使用する装備」にスロット番号を表示。
- 通常表示を最大6列に拡張。Start／Menuは他のOG操作に未割り当ての場合、列数を切り替えます。
- 登録名・検索欄を隠す「コンパクト表示」を追加。フォロワー操作は幅に余裕がある場合に同じ行へまとめます。
- 最大2人のフォロワーに自動着替えを追加（試験機能）。対象別に割り当てと待機時間を設定できます。
- 移動後の装備確認と、同じ服装を継続する場合の早期再適用に対応。着替え可能なプレイヤーを待機中のフォロワーより優先します。
- 整理画面の選択画像を右クリック、または上段のごみ箱から一括移動。ごみ箱内では一括復元にも対応。
- ギャラリーの選択枠のずれを修正。
- L2／R2で分類を切り替えた直後から画像を選択・移動できるように改善。
- 「ごみ箱を空にする」「完全に削除」の日本語表記を追加。
- ゲームパッド長押し起動時、起動ボタンでそのまま閉じてしまう現象への対策を追加。

## Changes in 1.0.14

- Added a wide gallery organizer with multiple selection, group moves, destination previews and 4–30 columns. Missing-set checking is now in the organizer.
- Added organizer-only name visibility and reduced spacing when names are hidden. Successful moves clear the selection.
- Improved recognition during Sofia's free-time behavior and allowed more everyday follower activities.
- Fixed text-input focus loss and gamepad double-step navigation; held-button navigation remains available.
- Full-outfit management now selects equipment to wear: checked items form a full replacement, unchecked items are excluded. Head/accessory presets remain partial changes. An empty selection does nothing.
- Added a 2–10 second minimum combat switch interval (default/recommended: 5). Weapon stabilization and attack/casting waits remain.
- Reopening the gallery returns to the normal outfit view, respecting the independent-preview setting.
- Fixed some equipment added by patches being incorrectly reported as missing, including support for affected existing presets.

Lower intervals may increase load or interactions with other mods. Existing photos, presets and settings are retained. Back up data before updating; restore pre-update data when downgrading.
If a full outfit uses an old edited exchange-slot selection, resave its selection in Equipment Management before assigning it to automatic switching.

## 1.0.14 の更新内容

- ギャラリー整理画面を追加。複数選択・まとめて移動・移動先表示・4～30列に対応。不足装備セット確認を整理画面へ移動。
- 整理画面専用の名前表示ON/OFF、非表示時の余白削減、配置変更後の選択自動解除。
- フォロワーの日常動作中の制限を緩和。Sofiaの自由行動中の認識に対応。
- 文字入力のフォーカス喪失とパッドの二重移動を修正。長押し移動は維持。
- 全身セットは「着替えに使用する装備」を選択する方式に変更。チェックした装備で全身を入れ替えます。頭装備・小物は部分交換のままです。全解除時は何も変更しません。
- 戦闘時の最短切り替え間隔を2～10秒に設定可能。初期値5秒（推奨）。判定安定待ち・攻撃や詠唱中の待機は維持。
- 再起動時は通常の着替え画面を表示。独立プレビュー設定は維持。
- 一部のパッチ由来の装備が不足扱いになる問題を修正。該当する登録済みセットにも対応。

短い間隔は環境により負荷や他MODとの干渉が目立つ場合があります。
写真・登録・設定は引き継ぎます。旧版へ戻す場合は更新前のデータも復元してください。
従来の全身セットの交換スロットを編集済みの場合、自動着替えに使う前に装備管理で選択を保存し直してください。

# Outfit Gallery 1.0.13 — 2026-10-09

## 日本語

1.0.12以降の試験版で調整した機能と修正を統合しました。

- 場所・環境・水中・スニーク等の条件別自動着替えを追加。屋内のお店にも対応。
- 戦闘ベースと近接・魔法・弓・格闘の自動切り替えを追加（試験的）。
- 鍛冶・錬金・付呪の設備使用時の着替えを追加（試験的）。
- 自動着替え自体のON/OFF用キー、修飾キー、パッド長押し、待機時間設定を追加。
- 全身プリセットの装備追加・交換、初回編集前への復元を追加。
- 小物登録と交換対象の選択でスロットなし防具に対応。
- 装備管理・編集の一覧を可変高さへ変更。
- オプションの配置、戦闘カード表示、フォロワーと不足装備の説明を整理。
- フォロワー選択と服装維持人数を上部に表示。
- 生成装備の受け渡し・追跡と、ごみ箱の関連ファイル処理の修正を統合。

設定・写真・登録内容は保持します。小物の新形式と旧版へ戻す際の制限はREADMEを参照してください。

## English

Consolidates the post-1.0.12 trials: conditional automatic outfits, shops, experimental combat styles and crafting, a dedicated automation toggle hotkey, modifier/hold bindings, preset content editing and initial restoration, individually selectable zero-slot armor, responsive management lists, streamlined options, follower selection/count display, and generated-item transfer/tracking and related trash-file fixes.

No gameplay changes from layout-test1 (DLL 1.0.12.44); only release version metadata/log labels and distribution documentation changed. Release DLL version: 1.0.13.0.
