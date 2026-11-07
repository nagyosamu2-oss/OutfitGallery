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
