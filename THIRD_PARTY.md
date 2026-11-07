# Dependencies and provenance

The implementation in `src/` and `tests/` was authored for this project with substantial AI assistance. It is provided under GPL-3.0 (`LICENSE`). No implementation code or artwork from Show Player In Inventory, Character Menu SE, or Skyrim Outfit Equipment System NG was copied into this project.

## Runtime1 instruction decoder

- `third_party/hde64/` contains Hacker Disassembler Engine 64 from MinHook (Vyacheslav Patkov; MinHook distribution by Tsuda Kageyu). It is used only to decode instruction boundaries, not to install MinHook detours. Retained BSD notices: `LICENSES/HDE64-MinHook.txt`. Upstream: <https://github.com/TsudaKageyu/minhook/tree/master/src/hde>.
- The SE/AE call identities were cross-checked against the pinned Fitting Room and Skyrim Fitting System sources recorded in `TRANSMOG-SKIN1.md`, and CommonLibSSE-NG v10.1.0. The new bounded resolver and tests are project code; no game executable or Address Library database is redistributed.

## Public integration headers

Exact upstream revisions are recorded in `third_party/upstream.json`; the downloaded files are retained unmodified.

- **SmoothCamAPI.h** — mwilsnd, SkyrimSE-SmoothCam, `SmoothCam/include/SmoothCamAPI.h`. The header explicitly invites mod developers to copy the file to use the API. Only this integration header is included; the SmoothCam implementation/DLL is not redistributed.
- **TrueDirectionalMovementAPI.h** — Ersh, TrueDirectionalMovement, `src/TrueDirectionalMovementAPI.h`. The header explicitly invites mod developers to copy the file to use the API. Only this integration header is included; the TDM implementation/DLL is not redistributed. A scoped macro adapter in our source selects the Windows ANSI module-name lookup expected by the header; the upstream file is unchanged.
- **SKSEMenuFramework.h** — QTR-Modding, SKSE-Menu-Framework-3-API. LGPL-2.1, see `LICENSES/SKSEMenuFramework-API.txt`. The framework is a separate user-installed dependency and is loaded dynamically. Its source is at <https://github.com/QTR-Modding/SKSE-Menu-Framework-3>; the API source is at <https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API>. This source distribution contains the complete API header used to build the plugin.

## Build dependencies

- CommonLibSSE-NG, alandtse/CommonLibSSE-NG tag v10.1.0. GPL-3.0-or-later with upstream Modding and linking exceptions. Built from unmodified official source with MSVC 19.44. See `LICENSES/CommonLibSSE-NG.txt` and `LICENSES/CommonLibSSE-NG-EXCEPTIONS.md`. <https://github.com/alandtse/CommonLibSSE-NG/releases/tag/v10.1.0>. CommonLib patch-safety support is disabled in the existing build; the transmog implementation performs its own instruction/callsite and hook-chain checks before installing its two call hooks.
- spdlog / fmt / DirectXTK / DirectXMath / rapidcsv: notices in `LICENSES/`. These are build dependencies; their corresponding license texts are retained from the installed packages.
- OpenVR headers/import library are the ones accompanying the official CommonLib v4.39.3 prebuilt bundle. They support CommonLib's multi-runtime compilation; this plugin admits the seven runtimes listed in README.md; additional targets remain untested. See `LICENSES/OpenVR.txt`.

Development testing was reported on Steam 1.5.97 and 1.6.1170, with the main feature-testing environment on 1.6.1170. See README.md for the supported-target/tested-runtime distinction. The implementation was produced with substantial AI assistance; retain this disclosure when preparing the public listing.

References:
- <https://github.com/mwilsnd/SkyrimSE-SmoothCam>
- <https://github.com/ersh1/TrueDirectionalMovement>
- <https://help.nexusmods.com/article/28-file-submission-guidelines>

JSON persistence uses nlohmann/json (MIT); notice: LICENSES/nlohmann-json.txt.

