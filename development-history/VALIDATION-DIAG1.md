# Validation — skin13-diag1 (1.0.17.14)

2026-11-02. Based on the existing, unfinished skin13 source, copied into a new directory.

- RelWithDebInfo x64 build completed successfully with the existing VS2022/MSVC 14.44 toolchain and CommonLibSSE-NG 10.1.0 craft bundle. No dependency replacement or preparation script was used.
- All 40 existing regression tests passed across the final scoped runs: 39 in the workspace test-temp directory, and TrashVirtualPathSafety separately with permission to create its junction fixtures. The first sandbox run failed temporary-file operations; moving TEMP/TMP into the workspace resolved those failures except junction creation. No test logic was changed.
- Exported DLL metadata verified as 1.0.17.14; the existing compatibility list is unchanged. The transmog hook remains gated to Steam 1.6.1170.
- Original skin13 files verified against pre-copy SHA256 hashes. No original file changed.
- Source comparison: changes are diagnostic-only additions in Transmog.cpp, DLL version in main.cpp, matching metadata assertion, and new diagnostic documentation. TransmogIdentity.h, its tests, Equipment.cpp, FollowerOutfits.cpp, presets, UI and CMakeLists.txt are byte-identical to skin13.
- Player first-person exclusion, follower Biped eligibility, skin/armor staging, identity restoration, hair mask and hide logic are unchanged from skin13.
- New observations are follower-specific and limited per application: four pass decisions, two hair-mask results, one eligible staging trace and two delayed snapshots. Model paths are logged without loading additional models. Delayed snapshots use the existing game-thread tick; no worker or task retry loop is introduced.

No game was launched and no MOD was installed automatically. Engine rendering, follower visibility, wig visibility and CTD behavior remain unverified. These software tests do not exercise the game's rendering pipeline. This is a diagnostic build, not a confirmed fix.

Build dependencies remain those of the original project: CommonLibSSE-NG 10.1.0 craft bundle, the recorded vcpkg static-md prefix, DirectXTK, spdlog and nlohmann-json. The source archive includes the unchanged project, tests, third-party notices and the diagnostic build command. It does not redistribute the external toolchain/dependency installation.
