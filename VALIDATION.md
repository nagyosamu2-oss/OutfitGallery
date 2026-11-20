# Validation — 2.2.9

Based on user-confirmed cold-test5. Only main.cpp release labels/metadata,
CMake project version and release documentation differ from that baseline.
All other functional source, tests, translations, dependencies and default INI
must remain byte-identical. Packaging requires Release build and 64 passing
selected tests. TrashVirtualPathSafety remains excluded for its known junction
fixture environment limitation and is not counted as passing.

User confirmed organizer usability, Wear set, automatic cold switching,
ordinary area -> forced snow -> Cold exposure, and final caption readability.
These are targeted confirmations, not validation of every runtime or modlist.
Native cold conditions accept bounded read-only GetPos, GetGlobalValue and
IsPlayerInRegion. No Survival settings, quests or global values are rewritten.
Standard switching continues when optional cold data is unavailable.

Prior 2.2.8 verification and FSMP limitations remain applicable; see archived
release documentation and development-history for previous checks.
The 2.2.4 external generated-item retention report remains unresolved. The
received diagnostic segment contains no generation, owned=1 and tracked=0.
Basic generation/tracking code is shared with that version; no fix is claimed.

Release preparation does not install, sync GitHub, commit, push or upload.
Prior ZIPs are preserved. No user data or official game scripts are packaged.
