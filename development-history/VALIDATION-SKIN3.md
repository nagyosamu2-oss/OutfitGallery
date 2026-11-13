# Skin3 validation — 2026-11-01

- MSVC 19.44 / RelWithDebInfo build passed.
- CTest: 40/40 passed. These cover existing isolated policies, not native renderer attachment.
- PE metadata check passed: 1.0.17.3. Appearance-only experiment remains gated to Steam 1.6.1170.
- Package script verifies the packaged DLL matches the built DLL and contains no INI/JSON settings replacements.
- In-game result and performance are NOT yet verified.

## Scope

Compared with skin2, collect worn ARMO identities from inventory entries once per eligible third-person rebuild; restore those identities only for staged slots, with actor skin as fallback when no worn armor occupies that slot. Do not use an ARMA secondary-slot marker as the original armor identity. Count deltas are not used to reject worn entries because they are not total inventory counts.

No new mask hook, first-person styling, polling timer, inventory mutation or equipment operation. Hair handling and unavailable-model errors remain unchanged. The entry scan adds work during rebuilds; its runtime cost has not been measured.

## Evidence and hypothesis

Skin2 Outfit64 slot53 comparisons showed no attached clone after restoring a body ARMA marker, versus an attached clone when restoring real slot53 ARMO. This experiment tests that correlation; it does not prove the native engine branch responsible.

## ゲーム内の確認

ゲーム終了後、skin2を無効化してskin3へ切り替えてください。テスト用セーブを使い、自動着替えは一時停止してください。

1. 実際にはスロット53装備を着けず、Outfit64を「外見のみ」で適用。パンツと身体の穴を確認。
2. 「実際の装備の外見に戻す」で復元を確認。
3. 実際にスロット53装備を着け、同じOutfit64を適用して比較。
4. 他の部位・一人称・復元も確認。以前のCTD確認はOutfit174→166。

結果と、ゲーム再起動前のOutfitGallery.logを共有してください。CTDがあればCrashLoggerも必要です。ログのIDENTITY3が今回追加した復元情報です。
