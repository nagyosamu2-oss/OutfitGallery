# Validation — 2.0.1

The user tested 2.0.1-rc2 with ImprovedCameraSE 2.0.0.2231 and confirmed first-person Gallery entry, return to first person after closing, and normal camera operation. Earlier hud3 tests by the user and a friend confirmed native cursor suppression, HUD hiding/restoration, and clear text at normal UI size under DLSS Quality. The user also confirmed the standalone Steam 1.7.104 transmog fix.

The final DLL changes the candidate's version identifiers, startup log, and Framework page footer. Its gameplay implementation matches the user-tested rc2 candidate. The final-numbered DLL has not been launched in-game by the assistant.

The supported targets are Steam 1.5.97 / 1.6.353 / 1.6.640 / 1.6.1130 / 1.6.1170 / 1.7.104 and GOG 1.6.1179. Gameplay confirmation is limited to the reported environments. GOG and the remaining target combinations are untested.

Automated checks exercise persistence, callsite selection, input policy, capture, translation and control ownership. Results for the final build are recorded in BUILD-CHECKS-2.0.1.txt. The known TrashVirtualPathSafety junction fixture cannot be created in this build environment and is excluded from the reported suite.

Upscaler compatibility mode defaults to OFF and is persisted in the studio settings JSON. The existing one-time follower-targeting migration remains version 1.
