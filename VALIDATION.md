# Validation — 2.2.1

Based on OutfitGallery-2.2.0-next-update-test1. Release preparation changes version metadata, footer/log labels and documentation only. Every other native source file and all tests, integration headers, translations and licenses are byte-identical to that candidate.

The user confirmed the random try-on improvements, tab navigation and photo pause/resume behavior, including release on gallery close. Failure diagnostic integration has automated/build coverage only; it has not been exercised in-game. It records bounded startup failure details for opt-in detailed logging and does not change hook acceptance.

The final 2.2.1-numbered DLL requires an in-game smoke check. See BUILD-CHECKS-2.2.1.txt for build/test results. No additional runtime gameplay coverage is claimed; the accepted runtime list is unchanged.

ApparelPreview incompatibility is a user-reported result, not a verified internal cause. No compatibility implementation is added in this release.
