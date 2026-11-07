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

## Updating / 更新

Close Skyrim and install the complete 2.0.1 package. Disable older trial/candidate DLL overrides and the standalone 1.7.104 Transmog Fix. Preserve customized INI, generated photos, presets, JSON settings, and paired .ess/.skse saves.
ゲームを終了して完全版パッケージを導入し、以前の試作版・候補版と単独の1.7.104修正パッチを無効にしてください。カスタムINI、生成済みの写真・プリセット・JSON設定、対応する.ess/.skseを保持して更新してください。

Upscaler compatibility mode defaults to OFF for new and existing settings without the new field. Users of hud1/hud2/hud3 should enable it once in Options if they want to keep that HUD behavior.
アップスケール互換モードは新規・既存設定とも項目未保存時はOFFです。hud1/hud2/hud3と同じHUD方式を使う方は、オプションで一度ONにしてください。

## Runtime coverage / ランタイム

Steam 1.5.97 / 1.6.353 / 1.6.640 / 1.6.1130 / 1.6.1170 / 1.7.104
GOG 1.6.1179 (untested / 実機未検証)

User testing includes the 1.7.104 transmog fix, DLSS Quality UI/HUD/cursor behavior, and first-person Gallery entry/exit with ImprovedCameraSE 2.0.0.2231. Other combinations remain untested. See VALIDATION.md.
1.7.104の重ね着修正、DLSS Qualityでの文字・HUD・カーソル表示、ImprovedCameraSE 2.0.0.2231での一人称からの起動・視点復帰を利用者が確認しました。全組み合わせの実機検証は実施していません。

GPL-3.0. Matching source and third-party notices accompany this release. Authored with substantial AI assistance.
