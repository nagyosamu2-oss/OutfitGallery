# Validation — 1.0.16

Based on user-tested photo-test4 and accepted name-compat5 cleanup. User confirmed photo replacement and cleanup option explanations. Preceding tests confirmed generated renamed-item removal, favorite/enchantment/tempering protection, preowned-item retention and outside-inventory retention. Chest-returned tracked items may be reclaimed; dropped items that lose identity remain untracked.

No change to the policy that skips cleanup when outfit verification fails. Broader cleanup behavior and the old-save CORE Carrier registration issue are deferred. No new monitoring or equip delay is added for release integration.

Release source differs from photo-test4 only in release/build labeling and documentation. The final release binary has not been launched in-game by the assistant. Not all supported runtimes or mod combinations have been gameplay-tested. Diagnostic name logging remains opt-in and bounded; it is disabled when detailed logging is OFF.

Release build: all 35 automated tests passed. Exported 1.0.16.0 metadata and the unchanged exact runtime list passed verification. Runtime/source ZIP integrity and packaged DLL byte equality were verified.
