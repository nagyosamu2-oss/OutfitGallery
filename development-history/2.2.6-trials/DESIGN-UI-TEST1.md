# UI test1

Base missing-item-test1. Native diff limited to GalleryBrowser.inc and main.cpp version labels (2.2.6.2). Opening serial sets a pending flag; next rendered Options header is explicitly closed once with SetNextItemOpen/Always. Keep pending through photo-retake suppression and organizer early return; do not write settings or reset every frame.

Remove favorite badge filled circle. Preserve star geometry, size, gold outline and position. Add a semi-transparent dark stroke behind the same outline for contrast without covering the interior. Other circular badges are unchanged.

No equipment, missing-item, cleanup, settings or serialization changes. Existing 60 selected tests retained; no implementation-mirroring UI tests added. Visual/runtime validation pending.
