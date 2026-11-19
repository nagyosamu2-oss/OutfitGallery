# Validation — 2.2.8

Based on user-confirmed2.2.7-native-hair-test1 (metadata2.2.7.3). Source changes
for this release are main.cpp version labels, CMake project version, one
FSMP diagnostic word (trial -> build), and release documentation only.
All functional code, tests, translations, dependencies and default INI match
the confirmed trial. 64 selected automated checks are required by packaging.
TrashVirtualPathSafety remains excluded for its known junction-fixture
environment limitation and is not counted as passing.

User runtime confirmations2026-11-19:
- Actual Painter Hair -> Yuna appearance, clear and reapply: works.
- Actual Yuna -> Painter Hair appearance: works in hair-test2.
- With FSMP native-hair-on-wig suppression OFF, native hair moves (workaround).
- With that FSMP option ON, native-hair-test1 restores native-hair SMP while
  keeping the real wig equipped; wig appearance and clear also work.
- User explicitly confirmed save in a native-hair appearance, full game exit,
  restart/load, native hair moving, then clear restoring the real wig.
- Native trial log independently confirms FSMP bridge installation and
  alternating scoped override=true/false during appearance changes.

The FSMP bridge is limited to the measured4.1.1 AE AVX image and game1.6.1170,
player only. Matching image metadata AND target function SHA256 are required.
Unknown CPU variants/runtimes, altered targets or unavailable near memory
decline only the optional bridge. The ordinary mod runtime list is unchanged.
No claim of universal SMP or every-wig compatibility. Native continuation tests
execute the bridge against a synthetic function, checking both trailing
arguments, return value and scoped flag restoration; they are not game tests.

Prior cumulative checks retained: missing-base equipment filtering, preserving
saved preset data, missing-enchantment fallback preserving smithing, clear
localized warnings and generated-item reclaim. Earlier comparison showed12/12
equipment verifications,100 verified reclaims and three27-effect recreations
with performanceEquivalent=true, without errors or ambiguous matches.

Earlier timing samples showed no evident broad slowdown; they are manual
override-to-restoration log intervals, not click-to-visible measurements and
do not prove a speedup. The FSMP bridge's runtime overhead has not been timed.
The outside generated-item retention report without a diagnostic log remains
unresolved and is not claimed fixed. Identity/reclaim ownership rules unchanged.

UI confirmations retained: Options reopening collapsed, main favorite star,
organizer badges and double-click photo pause without extra right-side buttons.
Some automatic/follower/crafting fallback combinations, all-missing runtime
cases and every supported runtime/mod combination remain unverified.

Packaging does not install, sync GitHub or publish/upload. Previous releases,
both hair checkpoints and experimental archives are retained unchanged.
