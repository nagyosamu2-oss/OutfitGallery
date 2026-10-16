#pragma once
#include <atomic>
namespace Gallery {
// Compatibility persists in Settings.json; diagnostics deliberately do not.
inline std::atomic<bool> cleanupCompatibility{false};
inline std::atomic<bool> reclaimRenamedGeneratedItems{false};
inline std::atomic<bool> detailedLogging{false};
inline bool DetailEnabled() noexcept {return detailedLogging.load(std::memory_order_relaxed);}
}
