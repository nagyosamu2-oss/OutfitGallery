# Outfit Gallery 2.2.1

## 2.2.1

- Improved random try-on matching for part names and A/B/C variants, reducing omitted matching pieces.
- L1/R1 tab switching now moves focus to the outfit list. Left/right navigation while focused on the tabs is unchanged.
- Added a photo pause/resume button beside camera preset 7. Blue indicates the pause action; orange indicates an active pause. Camera and gallery controls remain available. Closing the gallery releases pauses started by this feature.
- Added failure diagnostics to detailed logging for transmog startup validation. Hook acceptance rules are unchanged.

Known compatibility report: Transmog has been reported unavailable with ApparelPreview (Nexus 185334). Disabling ApparelPreview restored transmog in the reported setup. Compatibility remains unverified; no compatibility bypass is included.

## 2.2.1

- おまかせ試着の部品名・A/B/C差分の判定を改善し、同じ系統の部品の取りこぼしを軽減しました。
- L1/R1で分類タブを切り替えた後、そのままセット一覧を操作できるようにしました。タブ上での左右操作は従来どおりです。
- カメラプリセット7の右に、撮影用の一時停止・再開ボタンを追加しました。通常は青、停止中はオレンジで表示します。停止中もカメラとギャラリーは操作でき、終了時にはこの機能による停止を解除します。
- 重ね着の起動時検証に失敗した際の情報を「詳細ログを記録」で確認できるようにしました。重ね着のフックを受け入れる判定は変更していません。

互換情報：ApparelPreview（Nexus 185334）との併用時に重ね着が利用できなくなる報告があります。報告環境ではApparelPreview自体の無効化で改善しています。互換対応は未確認で、制限の回避処理は追加していません。
