# Outfit Gallery 2.2.4

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

自作付呪・独自名称・毒は新しい品へ複製しません。自作付呪品を所持している場合の優先使用は従来の付呪優先機能です。生成後に追加強化・自作付呪した品は回収から保護します。識別できない品を同名という理由だけで一括削除することはありません。以前の版で回収対象として記録されなかった残留品を、一律に削除する変更でもありません。

### 更新方法

Skyrimを終了し、旧版や試験版のDLLよりこの公開版が優先されるよう導入してください。写真・プリセット・Library.json・設定・カスタムINI、対になった.ess/.skseを保持してください。通常利用にニューゲームは不要です。更新前のデータを保管してください。旧版では新しい強化情報を保持できない場合があります。対応ランタイムと必須環境は従来どおりです。

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

Custom enchantments, custom names and poison are not recreated. The existing enchantment-preference option uses owned enchanted items. Items further improved or custom-enchanted after generation are protected from cleanup. Unidentified items are never mass-deleted by base type, and previously untracked leftovers are not retroactively adopted for cleanup.

### Updating to 2.2.4

Exit Skyrim and install this release with priority over earlier trial DLLs. Keep photos, presets, Library.json, settings, custom INI and paired .ess/.skse saves. Back up the existing data before updating. No new game is required. Older versions may not retain the new smithing metadata. Runtime requirements and the supported executable list are unchanged.
