# Validation: skin11
- RelWithDebInfo build succeeded, 2026-11-02.
- Existing CTest regression suite: 40/40 passed.
- Exported version 1.0.17.11 and runtime metadata verification passed.
- Manual and automatic player mode use a single atomic value. UI requests mode changes through the game-thread command queue.
- Switching modes resets automatic condition policies; selecting the current mode does not reset them.
- Follower mode controls are disabled; follower automatic outfits remain normal equipment.
- In-game UI appearance and automatic switching still require user validation. Existing tests do not exercise game rendering.
