# Experimental FSMP native-hair physics bridge

Base: confirmed 2.2.7-hair-test2 checkpoint. DLL metadata2.2.7.3.
This is a separate prototype, not a change to that checkpoint or a public release.

## API investigation

FSMP's PluginAPI.h interface2.0.0 exposes physics pre/post-step listeners,
not a per-actor native-hair visibility override. Its contract only permits
forces/torques in pre-step and read-only observation in post-step; neither
listener is used to mutate activity. Installed DynamicHDT.psc exposes bone
kinematic toggles and mesh resets, neither of which replaces the head-active
wig predicate. No Papyrus calls, invented API messages or global config edits
are added. The installed DLL exports only its three SKSE loader entry points.

Primary source references read2026-11-19:
- https://github.com/DaymareOn/hdtSMP64/blob/dev/src/PluginAPI.h
- https://github.com/DaymareOn/hdtSMP64/blob/dev/src/ActorManager.cpp
- https://github.com/DaymareOn/hdtSMP64/blob/dev/src/dhdtPapyrusFunctions.cpp

## Exact installed-binary measurements

Measured Faster HDT-SMP4.1.1 AE AVX (not AVX2/AVX512/SSE2):
- PE timestamp6A8F44A9, SizeOfImage41B000.
- DLL SHA256 FF9C7F3F78ECA122AC0AA9BD58A0628D7200685F86EF63FBD18BFE7F2C6D667F.
- RSDS F76F8A22-A848-4E46-B6AB-74572D0FF3B2, age1; shipped hdtsmp64.pdb.
- Matching PDB: Skeleton::updateAttachedState(const RE::NiNode*,bool) returns
  bool; RVA92790, length372hex. This includes both real trailing arguments,
  not the zero-argument signature a superficial reading might suggest.
- Skeleton owner pointer offset0, head offset18, head.headNode offset28,
  head.isActive offsetD9: absolute offsets40 andF1hex.
- Disassembly reads F1 at RVA92924 and updates headpart physics only when that
  flag AND the skeleton's own active flag108 are true. It does not write F1.
- ActorManager FrameEvent RVA86B90 holds its recursive mutex across
  setSkeletonsActive at914D0. That routine calls updateAttachedState at91992.
  Menu/config/skin event paths likewise hold ActorManager's mutex.
- Whole target function SHA256
  EA61D300D99616B9137A32227C33BE77A740F9E1BE02B625893DF54F1BB8A81F.
  The PE relocation table contains no relocation in this function.
- Stolen first10bytes are exactly two non-RIP-relative stack stores:
  48 89 5C 24 10 / 48 89 6C 24 18. No calls, branches, stack adjustment or
  relative operands require relocation. Continuation executes the third store
  and the remainder of the original function unchanged.

These facts were read from THIS DLL/PDB, not inferred from upstream dev source.
Measurements and read-only inspection tools are in work/hair-investigation;
no FSMP DLL/PDB or user log is redistributed.

## Behavior and ownership

The immutable appearance snapshot caches whether31 is controlled by the look
and neither head30 nor hair31 is covered by its valid appearance addons. All
four new/edited/restored-look publish paths recompute this cached decision;
existing immutable try-on snapshots retain it. Clearing/loading/race teardown
already clears the look through the existing lifetime paths.

The bridge wraps only FSMP's existing updateAttachedState. Under its existing
mutex and only for the player with a tracked head, an active native-hair look
temporarily lends head.isActive=true if its original value is false. The
original method performs all activation and preserves its original scene,
camera, distance, budget, first-person and deactivation decisions. A scope
guard restores the original flag immediately after that synchronous call,
including exception exit. No Skeleton/node pointer is saved past the call.

Other actors and inactive/wig/helmet/unowned appearances call the original
without a flag store. It adds no actor scan, inventory walk, scene walk,
equipment event, global reset, skeleton rebuild or background poll. A cached
appearance decision is read from the existing atomic immutable look registry
inside FSMP's own update cadence. Diagnostics log only player override changes.

## Installation guard and code forwarding

The prototype requires game1.6.1170 plus the exact PE timestamp/image size and
SHA256 of the loaded target function. Unknown CPU builds, unknown versions,
missing FSMP and already-modified target functions are declined. No foreign
detour chain is guessed. The decision provider is installed only after the
normal Outfit Gallery transmog hooks succeed.

A separately allocated near page preserves the ten measured entry bytes then
jumps absolutely to entry+10. A RIP-relative indirect jump forwards to the
three-argument wrapper without clobbering registers. The page becomes RX before
publication, caches are flushed, and the entry is checked again immediately
before its ten-byte patch. Allocation/protection failure declines this optional
bridge instead of aborting. This uses no new game address or game hook and
does not consume/recreate the existing SKSE trampoline. Continuation storage
lives for the process lifetime; runtime DLL unload is not supported.

## Validation and outstanding game check

Policy/scope tests cover native/wig/helmet/unowned looks, original false/true
flags, disabled binding, exception restoration and invalid flag values.
Continuation tests check displacement bounds and exact encoding and EXECUTE
the same bridge against a synthetic x64 function inside the test process,
checking both trailing arguments, return value and post-call flag restoration.
They do not load FSMP or alter Skyrim.

The real game check MUST use FSMP's wig-disables-native-hair setting ON and
restart. OFF already works without this prototype and cannot validate it.
Visual physics outcome and save/load recovery remain unverified until the
user's test. This is not official FSMP API support or support for all4.1.1 CPU
builds. The confirmed checkpoint remains the fallback.
