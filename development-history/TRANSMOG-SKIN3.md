# Transmog skin3 — identity experiment
DLL version 1.0.17.3. Steam Skyrim 1.6.1170 only for transmog.
Based on skin2. Only staged third-person slot identity restoration changes.
Worn ARMO identities are collected once per eligible rebuild. Empty worn slots
use actor skin, rather than restoring a prior ARMA secondary-slot marker.
Untouched slots and shield remain untouched. First person remains real gear.
No mask hook, new timer, inventory mutation or automatic equipment changes.
This is not yet verified in-game. Hair and unavailable-item handling are unchanged.

Exit Skyrim; disable skin2 in MO2; install this as a separate trial override.
Use a test save and pause automatic outfits. Apply Outfit 64 with real slot53
gear absent, restore, then repeat with real slot53 gear present. Check pants,
body, other parts, restoration, and first-person view. Also check Outfit174 to
Outfit166 for the earlier CTD. Send OutfitGallery.log; if CTD, CrashLogger too.
Logs add IDENTITY3 type/owner choices; capture before restarting Skyrim.
References: Fitting Room RealWorn.h and BipedPost.cpp, pinned commit
8717ae7a8fe6185a65721564bac7237ddd411f65 (GPL-3.0). Existing notices retained.
