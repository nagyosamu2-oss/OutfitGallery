# Building Outfit Gallery 1.0.16

Use Windows x64, Visual Studio 2022 C++ tools (tested MSVC 19.44), CMake 3.24+, Ninja and C++23 support.

## Dependencies

- alandtse/CommonLibSSE-NG **v10.1.0**: https://github.com/alandtse/CommonLibSSE-NG/tree/v10.1.0 . Initialize its submodules recursively. Build from source; the official prebuilt archive uses a newer MSVC toolset than the tested build machine.
- vcpkg packages: `spdlog fmt directxtk directxmath rapidcsv nlohmann-json`, using `x64-windows-static-md`.
- Integration headers are included in `third_party/`; exact revisions and hashes are recorded in `third_party/upstream.json`. Keep them unchanged.

The dependencies are external, not a vendored or locked complete toolchain. The commands below rebuild the source but do not promise byte-identical binaries across different toolchains/dependency updates.

From an x64 VS developer shell, replace the example dependency paths:

```powershell
$env:VSLANG = '1033'
cmake -S . -B build -G Ninja -DSKSE_SUPPORT_PATCH_SAFETY=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCOMMONLIB_SOURCE="C:/deps/CommonLibSSE-NG-10.1.0" -DCMAKE_PREFIX_PATH="C:/deps/vcpkg/installed/x64-windows-static-md"
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
```

Output: `build/bin/OutfitGallery.dll` and `OutfitGallery.pdb`. Install the DLL with `OutfitGallery.ini` under `Data/SKSE/Plugins/`. PDB is optional debugging information, not required for normal play. There is no ESP or Papyrus component.

Keep CommonLib's normal multi-runtime compile options. This project's plugin metadata and load entry deliberately restrict accepted runtimes to the seven executables documented in README.md; building against CommonLibVR does not enable VR support.

`PortraitPngCrop` exercises WARP/D3D PNG capture, scaling and display-view color handling. `PresetPersistence` exercises JSON persistence, validation, collection scope, settings and input helpers. Neither test simulates Skyrim equipment events or controller/UI interaction.

The normal release archive contains the DLL, INI, user documentation and license notices. Publish the matching source archive alongside it. Keep generated photos, user JSON, game saves, local logs and build directories out of source control and release archives.


The release uses the same CommonLib 10.1.0 static bundle as layout-test1. The dependency source archive is included under dependencies in the source ZIP. Use COMMONLIB_SOURCE to build it with your toolchain. Set VSLANG=1033 so Ninja can track MSVC include dependencies correctly.
