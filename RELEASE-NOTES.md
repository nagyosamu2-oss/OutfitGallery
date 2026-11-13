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

## Variation groups and accessory tabs
Create an accessory tab in Options using Create tab. It accepts accessory presets, not full outfits; grouping does not change which slots a preset equips. New accessory registrations remain in the built-in Accessories tab; assign them to custom tabs afterward.

In Organize gallery, select matching variations and choose Group variations (also available on right-click). The leading photo represents the group. Save and close commits organizer changes; Cancel discards them. A representative has a +count badge. Hover for about 0.25 seconds to expand; leaving the source and strip for about 0.1 seconds closes it. Controller users can open with A, navigate left/right, and close with B or up/down. Right-click actions keep the strip visible. Overflow uses the available gallery width and horizontal scrolling.

In the organizer, the representative selects the whole visible group; individual expanded photos select independently. Detach removes only selected members. Grab the exposed top of the original representative to drag the group while expanded. Grouping is manual, for 2–100 compatible presets of the same type/slot scope. Search and tabs show only matching members. Grouping does not delete the original photos or merge equipment data.

Blank full-outfit registration names prefer the body item; other registrations use the first named registered item. Remove all / Restore all acts on the current removal-window list with verified sequential operations. Closing keeps the current state; refreshing resets its restoration list. Removing clothes can expose skin.

## 差分まとめ・小物タブの使い方
オプションの「タブを作成」から小物用のタブを作れます。全身セットは登録できません。小物として登録したセットの装備範囲を維持し、分類先を増やす機能です。新規の小物登録は標準の小物タブへ保存され、そこから追加タブに登録します。

「整理・並べ替え」で差分を複数選び「差分をまとめる」を実行します。右クリックからも操作できます。先頭の写真が代表になり、選択チェックは解除されます。「保存して終了」で確定、「キャンセル」で破棄します。

代表写真には「+件数」が付き、約0.25秒カーソルを合わせると展開、元写真と展開枠の外に約0.1秒離れると閉じます。パッドはAで開き、左右で選択、Bまたは上下で閉じられます。見切れる場合はホイール・パッドで横スクロールできます。右クリックメニューの操作中は展開を維持します。

整理画面では代表写真がグループ全体、展開内の写真は個別選択です。選択した差分だけ「まとめを解除」でき、実行後は選択を解除します。展開中も元の代表写真の露出した上部から掴むと、展開を閉じてグループ全体を並べ替えられます。

同じ種類・対象スロットのセットを2～100件、手動でまとめます。検索・分類に該当する差分だけを表示し、元の写真や装備データは削除・統合しません。

名前空欄で全身セットを登録するとボディ装備名を優先し、それ以外は登録した装備の最初の名前を使います。「すべて外す」「すべて戻す」は装備解除画面の現在の一覧を順に変更・確認します。閉じると現在の状態を維持し、一覧を更新すると復元リストを作り直します。服を外すと肌が露出する場合があります。

## Updating / 更新
Exit Skyrim and install the complete 2.2.0 archive. Disable earlier trial DLL overrides, including variations-test12, accessory-tabs and equipment-list trials. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. Back up Library.json before updating; saving with older releases can discard new tab/group metadata. No new game or preset re-registration is required. Existing external translations still work; untranslated new keys fall back to English.

Skyrimを終了し、完全版2.2.0を導入してください。test12などの試作DLL・旧差し替えパッチを無効にし、写真・プリセット・Library.json・設定・カスタムINI・対の.ess/.skseは保持してください。更新前にLibrary.jsonをバックアップしてください。旧版で保存すると追加タブ・差分の情報が失われる場合があります。ニューゲーム・再登録は不要です。外部翻訳の新規未翻訳項目は英語になります。

Supported runtimes and the optional upscaler compatibility default (OFF) are unchanged. This release does not claim new testing on every supported runtime. GOG 1.6.1179 remains untested.
