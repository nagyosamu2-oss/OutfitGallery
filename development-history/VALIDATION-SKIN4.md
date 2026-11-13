# Skin4 validation 2026-11-01

- MSVC19.44 RelWithDebInfo build passed.
- CTest40/40 passed. TransmogIdentity regression covers replacing slot31 both ways and preserving each of the other31 bits.
- PE metadata verification passed for1.0.17.4; appearance experiment remains Steam1.6.1170 only.
- Native head-part mask call validated against expected E8 target before patching; previously known AE1.6.1170 site REL24724+0x80 to REL16044. Unknown hooks are not chained.
- Original mask returned when inactive, nonplayer, or race/sex no longer matches. Only bit1 (editor slot31) changes during the active look. Selection changes use immutable look snapshot; clear removes it before refresh.
- Skin3 ARMO restoration retained. No new polling loop, inventory scan, or equipment operation added relative to skin3.
- Package script verifies DLL equality and absence of INI/JSON replacements.
- Engine head-part updates, actual hair rendering, and CTD regression are NOT validated by unit tests. Requires in-game63→12→63, clear, slot53, and174→166 checks.
- Prior mask-hook removal was part of CTD isolation, not a proven diagnosis. This trial reintroduces a narrower hair-only call hook; runtime stability remains to be tested.
