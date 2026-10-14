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

### Automatic follower outfits (experimental)

Enable follower selection, aim at an accompanying follower, open OG and switch to automatic outfits. Check the target name, assign full outfits and enable automation.

- Register up to two followers. Disabled configurations still occupy a slot; remove one under the registered-followers list to free it.
- Assignments and enabled state are stored when saving the game. Save after editing; loading restores the configuration in that save.
- Enabling automation disables fixed outfit maintenance for that follower. Turning automation off does not automatically re-enable maintenance.
- Conditions are evaluated for the follower: location, weather, time, sneaking, swimming and combat. Clothing/armor changes preserve weapons and spells. Missing inventory items are generated.
- Crafting outfits and the automation toggle hotkey remain player-only.
- Changes are processed sequentially. Travel recovery is bounded; it does not continuously override another mod's outfit changes.
- Enable detailed diagnostic logging in Options when investigating problems. Travel/equipment diagnostics are included. Logging resets to OFF on game restart; diagnostic inventory scans do not run while OFF.

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

### フォロワーの自動着替え（試験機能）

「フォロワー選択」を有効にして同行者に照準を合わせ、OGを開いて自動着替え画面へ進みます。上部の対象名を確認し、全身セットを割り当てて有効にしてください。

- 登録は最大2人。無効にした設定も枠を使います。「自動着替え登録中のフォロワー」から登録を解除すると枠が空きます。
- 割り当てと有効状態はゲームのセーブ時に保存します。変更後はゲームを保存してください。ロードすると、そのセーブの状態に戻ります。
- 有効化すると、その対象の「この服装を維持」を解除します。自動着替えをOFFにしても固定維持は自動で再開しません。
- 対象本人の場所・天候・時間帯・隠密・水泳・戦闘条件で衣服／防具を交換します。武器・魔法は変更しません。不足する所持品は生成します。
- 作業台の着替えと自動着替えの切替ホットキーはプレイヤー専用です。
- 着替えは順に処理します。移動後の服装復帰は回数を限定しており、他MODによる継続的な服装変更を常時打ち消す機能ではありません。
- 調査時はオプションの詳細ログを有効にしてください。移動・装備状態の診断も記録します。ゲーム再起動時はOFFに戻り、OFF中は調査用の装備一覧走査を行いません。

