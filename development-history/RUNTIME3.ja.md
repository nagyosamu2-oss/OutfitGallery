# Runtime3 — Skyrim Outfit System の導入判定による制限を解除（DLL 1.0.18.8）

## 変更内容

`SkyrimOutfitSystem.dll` / `SkyrimOutfitSystemSE.dll` が読み込まれているという理由だけで、Outfit Gallery の重ね着を無効にする判定を削除しました。
追加の確認画面や設定項目はありません。

ゲームの呼出位置・呼出先と IED の連携検証は維持します。実際の呼出先が未対応の処理に置き換わっている場合は、従来どおり重ね着の初期化を停止します。
FittingRoom / ApparelPreview / SkyrimFittingSystem に対する既存の判定は今回の変更対象外です。

通常着替え、重ね着の描画、一人称、フォロワー、保存復元、UI、初期値、対応バージョン一覧は引き継いでいます。
Runtime2 とそれ以前の作業・配布物は変更していません。

## 併用についての説明

他の重ね着 MOD と同じ対象の外見を同時に変更すると、表示が競合する可能性があります。同時使用時の互換性は保証していません。Skyrim Outfit System が導入されているだけでは、重ね着を無効にしません。

## 確認状況

- Runtime2 は利用者から Steam 1.5.97 で使用できたとの報告あり。保存した起動ログでも呼出位置の検証・フック設置・IED 連携の成功を確認しました。
- Runtime3 のビルドと検査結果は同梱の `BUILD-CHECKS-RUNTIME3.txt` を参照してください。
- Runtime3 で Skyrim Outfit System を導入した状態のゲーム内動作は未検証です。
- 他の対応バージョンでの確認状況は Runtime2 から変わりません。

## 入れ替え

ゲームを終了して Runtime2 の代わりに Runtime3 を有効にしてください。設定ファイルの削除・初期化は不要です。
Skyrim Outfit System を戻して確認する場合、まず重ね着ボタンの選択、適用、解除を確認してください。
選択できない場合は、その起動の `OutfitGallery.log` から実際の停止理由を確認できます。
