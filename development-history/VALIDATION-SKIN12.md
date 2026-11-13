# Validation: follower transmog prototype skin12

- RelWithDebInfo x64 build succeeded.
- Existing regression suite: 40/40 passed.
- Runtime metadata verified separately for Steam 1.6.1170.
- Registry snapshots retain their lifetime across hook/UI iteration.
- Manual full/head/accessory application, restore, normal equip, follower auto application and mode UI now use the selected actor.
- Appearance and mode are session-only and cleared at load/new-game boundaries.
- No in-game verification performed by the assistant. The existing tests do not exercise engine rendering or follower appearance isolation.

Required in-game checks: independent player/follower A/follower B looks and modes; full/head/accessory selection; wig restoration and UBE slot53; normal restore; automatic condition changes; fast travel; disabling/removing automatic follower registration.
