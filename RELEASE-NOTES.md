# Outfit Gallery 2.2.9

## 2.2.9 更新内容（2.2.8から）

- 整理画面の「表示／登録先」タブをクリックすると一覧を切り替え、写真をドロップすると分類へ登録します。選択中の写真があってもクリックだけでは登録しません。
- 整理画面のタブをメイン画面に沿った並びにし、頭装備・小物アイコンを追加しました。上部ごみ箱アイコンからごみ箱内を表示できます。
- 分類へドロップした後は処理済みの写真の選択を解除します。種類が合わず登録できなかった写真は選択に残します。
- 写真の右クリックに「登録する分類」を追加しました。メニューを広げ、カーソル付近へ表示します。パッドでは対象写真付近へ表示します。
- おまかせ試着に「セットを着用」を追加しました。現在表示中の試着セットを登録せず実装備にできます。
- 撮影用の停止／再開ボタンの説明に、右画面ダブルクリックの操作を追記しました。
- 自動着替えの表示を最大6列まで拡張しました。
- プレイヤーの自動着替えに「寒冷地」「寒さ対策」を追加しました。寒さ情報が未取得でも、通常の自動着替えは継続します。

### 防寒衣装の設定

「寒冷地」は公式サバイバルの凍える地域の区分だけで判定し、昼夜・天候には左右されません。「寒さ対策」は地域・夜・雨・雪などから公式サバイバル相当の環境の寒さを計算します。どちらもサバイバルモードONを要求しませんが、公式サバイバルのデータ（ccQDRSSE001-SurvivalMode.esl）が必要です。蓄積した冷えや装備の暖かさをそのまま再現する機能ではありません。

2枠のどちらか一方へ衣装を登録してください。片方へ登録すると他方は解除します。戦闘・睡眠・水中・スニーク・専用の屋内場所などを優先し、屋外では防寒衣装を通常の雪・雨・夜の衣装より優先します。両方未登録なら従来のルールで動作します。

説明は未登録枠の内側下部だけに表示し、写真を登録すると隠れます。寒さ対策の詳細はツールチップでも確認できます。

### 整理・試着の操作

整理画面の分類・並び・ごみ箱の変更は「保存して終了」で確定し、「キャンセル」で破棄します。頭装備・小物の組み込みタブは表示用で、ドロップでセットの種類を変換する機能ではありません。

「セットを着用」は実装備を変更します。重ね着として適用する操作ではありません。不足品の生成・性能の再現・生成品の回収には既存の設定と保護条件を使います。

### 更新方法・継続調査

Skyrimを終了し、旧版・試験版よりこのDLLが優先されるよう導入してください。写真・プリセット・Library.json・設定・カスタムINIと、対になった.ess/.skseを保持してください。ニューゲーム・セットの再登録は不要です。必須環境と従来の対応ランタイムは変更していません。2.2.8の髪・FSMP連携も従来の対応範囲で維持しています。

旧2.2.4で生成品が所持品に残るとの報告は継続調査中です。受領ログは生成後の所持済み装備と追跡対象0件を示しており、生成時の詳細記録がないため原因を特定できません。この更新で解決済みとは扱っていません。

---

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
