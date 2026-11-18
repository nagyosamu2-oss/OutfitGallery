# Outfit Gallery 2.2.5

## 2.2.5 更新内容

- 防具に付けた自作付呪をセットへ保存し、対象品を所持していない場合にも鍛冶強化と合わせて再現できるようにしました。プレイヤー・フォロワーに対応し、保存した内容はニューゲームでも利用できます。
- 複数効果のコピー付呪にも対応します（対応条件内・最大32効果）。
- 装備管理の付呪付き装備に「[付呪]」を表示します。
- 「強化・付呪を再現する」を追加しました。OFFでは、不足装備を基本性能で生成します。所持品の性能や登録済みの強化・付呪情報は維持します。重ね着には影響しません。
- 未選択のセーブで初めてギャラリーを開いた時に、再現方法を選べるようにしました。初回画面とオプションに「初回確認を省略」を追加しました。
- 再生成時にゲームが付呪の価格指定を変更した場合でも、効果・強化度などが一致していれば再現できるようにしました。元の品と価格が異なる場合があります。

### 登録・再現の使い方

強化・自作付呪した防具を実際に装備してセットを登録してください。既存セットでは、装備管理の「現在の装備から追加・交換」で登録内容を更新する方法も使えます。以前のセットへ付呪情報は自動追加しません。今回の試験版で保存した対応情報は引き継げます。

保存した性能を不足品へ再現する場合は、「不足している装備を追加して復元」「強化・付呪を再現する」「登録した付呪付き装備を優先する」をONにしてください。識別できる登録元の現物や、元セットに紐付く生成品・保護品を所持していれば再利用します。見つからない場合に保存情報から生成します。宝箱の元の品を移動する保管機能ではありません。

再現OFFは不足品の生成方法だけを切り替えます。元のMODの防具データに最初から付いている付呪は残ります。識別できる所持品や保護品の強化・付呪も取り除きません。ONに戻せば保存情報を再び利用でき、再登録は不要です。

初回の選択と後からの再現ON／OFFは、ゲームをセーブするとそのセーブに記録されます。「初回確認を省略」は全体設定です。ONなら、未選択のセーブは最後に設定した再現方法を使います。選択済みのセーブはその回答を維持します。未選択の古いセーブへ戻ると、再度確認される場合があります。

### 加工品の扱いと対応範囲

生成後に追加強化・付呪した品は回収から保護し、識別できる間は元セットで再利用します。加工内容はセットへ自動反映しません。新しい性能をニューゲームでも再現したい場合は、加工品を装備して登録内容を更新してください。このMODが追跡する生成品だけを回収し、同名という理由で元の所有品を一括削除することはありません。

対象は防具・衣服・装飾品の常時発動／自己対象の対応付呪です。最大32効果で、効果参照・数値・形式等に検証条件があります。個別条件付きの効果、一時的な付呪などは対応外です。必ず元の装備・効果を追加したMODも必要です。通常の1～2効果、複数効果のコピー付呪、鍛冶強化との組み合わせを確認しています。効果の範囲／持続時間が0以外の種類は実機未確認です。

独自名称そのものや他MOD独自の個体情報の完全な保存・復元は対象外です。名前だけを変更するMODと、品を別個体へ交換するMODでは挙動が異なり、互換性は個別確認が必要です。

### 更新方法

Skyrimを終了し、旧版・試験版より公開版DLLが優先されるよう導入してください。通常利用にニューゲームは不要です。写真・プリセット・Library.json・設定・カスタムINIと、対になった.ess/.skseを保持してください。旧版へ戻してセットを編集・保存すると新しい付呪情報が失われる場合があるため、更新前のデータを保管してください。必須環境・対応ランタイムは従来どおりです。

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
