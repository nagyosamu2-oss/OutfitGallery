# Outfit Gallery 2.2.8

## 2.2.8 更新内容（2.2.5から）

- 通常着替えで、元のMOD・装備本体が見つからない項目を除外し、残りの装備を適用できるようにしました。除外した装備名を表示し、登録内容は維持します。全項目が見つからない場合は、現在の実際の装備を変更せず停止します。
- 保存した自作付呪の参照先MOD・レコードが見つからない場合、その品は保存した鍛冶強化を維持し、自作付呪を省いて装備します。セットの残りも適用し、省いた品と参照先を日本語で案内します。
- 付呪再現の参照エラーに、不足プラグイン名や参照IDを表示するようにしました。
- オプションを開いたままギャラリーを閉じても、次に開いた時はオプションを折り畳んだメイン画面を表示します。設定値は維持します。
- メイン画面のお気に入りマークの丸背景を外し、星の輪郭と薄い影に変更しました。
- 整理画面の一覧・差分展開内にもお気に入りマークを追加しました。整理画面では見分けやすい丸背景付きの星を使い、右端から少し内側へ配置しています。登録・解除は保存前にも表示へ反映します。
- 右側の表示領域をダブルクリックして、撮影用の一時停止／再開を切り替えられるようにしました。独立プレビューON／OFFの両方に対応します。左側の既存ボタンも引き続き利用できます。

- 一部のウィッグを実装備にしたまま重ね着すると、重ね着先の髪が消える問題を修正しました。31番・41番にまたがる描画情報を扱い、Painter HairとYunaの双方向の切り替えで確認しています。
- 対応するFSMP環境では、実装備のウィッグを維持したまま、自毛を表示する重ね着でもSMPが動くように連携します。装備の付呪・防御力を維持し、FSMPの全体設定は変更しません。追加連携の対応範囲は下記を参照してください。

### 欠落装備の除外について

セットの保存内容は書き換えません。元のMODを戻すと、その装備も再登録なしで再び利用できます。残る装備は、保存した強化・付呪の再現設定に従って適用します。装備確認は利用可能な項目を対象に行い、確認後は従来の条件で生成品を回収します。

対象は通常着替えです。重ね着や装備管理の編集は従来の参照確認を維持します。装備本体が存在し、保存した自作付呪の参照先だけが欠ける場合は、自作付呪全体を省いて適用します。一部の効果だけを切り捨てて再構成する方式ではありません。元の登録データは維持し、提供MODを戻せば再登録なしで再現できます。識別できる所持品や元の防具データの付呪を削除することはありません。装備の種類や対象スロットの変更、付呪データの形式不正、個体識別の曖昧さなど、欠落した参照先以外のエラーは従来どおり停止します。不足品の生成には「不足している装備を追加して復元」が必要です。

### ダブルクリック操作について

キャラクターが表示されている右側の領域を、動かさず左ダブルクリックしてください。2回目を離した時に切り替えます。ドラッグした場合・領域の外で離した場合・右ボタン併用時は切り替えません。左ドラッグの位置調整、右ドラッグの回転・角度調整、ホイールの距離調整は従来どおりです。

このMODが停止した時間はギャラリーを閉じると再開します。他の機能が既に停止させていた時間は解除しません。今回、右側に新しい常設ボタンは追加していません。

### 重ね着の髪・FSMP連携について

ウィッグ同士の置き換えは、装備名や特定MOD名ではなく、実装備と描画パーツの対応を確認して処理します。髪以外も含む複合装備、所有関係が曖昧なもの、すべてのウィッグの互換性を保証するものではありません。

自毛SMPの追加連携は、現時点では**Skyrim1.6.1170＋解析済みのFSMP4.1.1 AE AVX版、プレイヤーのみ**が対象です。FSMPの同じ4.1.1でも、SSE2・AVX2・AVX-512版は今回の連携対象に含みません。DLLの識別情報と対象処理の一致を確認できた場合に自動で有効になり、対象外や対象処理が変更された環境では、この追加連携だけを無効にします。通常の着替え・重ね着の従来の対応環境は変わりません。FSMPの導入は必須ではありません。

対応環境では「ウィッグ着用時に髪の物理を無効化」がONでも、自毛を表示する重ね着中はその物理演算を維持します。ウィッグへの重ね着、解除、セーブしてゲーム再起動後のロードを確認しています。FSMPの設定ファイルや全体のON/OFF設定は書き換えません。対象外の環境で同じ停止が起きる場合は、このFSMP設定をOFFにする回避方法がありますが、隠れている自毛の演算も残る場合があります。

頭装備を実際に外す処理は追加していません。自毛の髪型を別のウィッグとして生成する方式でもありません。全身の物理リセットや追加の装備切り替えを行わず、対応する物理更新中の頭部パーツ判定だけを補います。

### 更新方法

Skyrimを終了し、旧版・試験版より公開版DLLが優先されるよう導入してください。写真・プリセット・Library.json・設定・カスタムINIと、対になった.ess/.skseを保持してください。ニューゲーム・セットの再登録は不要です。必須環境・対応ランタイムは従来どおりです。


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
