#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace Gallery::RandomTryOn {
// Selection is independent of display/filter order. Adding a multi-slot part
// removes every conflicting part, while unchecking only removes that item.
inline std::vector<bool> CustomizeSelection(const std::vector<std::uint32_t>& slots,
    std::vector<bool> selected, std::size_t index, bool enabled) {
    if (slots.size() != selected.size() || index >= slots.size())
        throw std::runtime_error("Invalid try-on selection.");
    if (enabled) for (std::size_t n = 0; n < selected.size(); ++n)
        if (slots[n] & slots[index]) selected[n] = false;
    selected[index] = enabled;
    return selected;
}
}
