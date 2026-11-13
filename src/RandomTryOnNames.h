#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace Gallery::RandomTryOn {
inline std::string LowerAscii(std::string_view text) {
    std::string result(text);
    for (auto& ch : result)
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    return result;
}

struct OutfitName {
    std::string family;
    std::string_view part;
    std::string_view variant;
    std::string number;
};

inline std::string SeparateOutfitWords(std::string_view text) {
    const auto upper = [](char ch) { return ch >= 'A' && ch <= 'Z'; };
    const auto lower = [](char ch) { return ch >= 'a' && ch <= 'z'; };
    const auto digit = [](char ch) { return ch >= '0' && ch <= '9'; };
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        // WereHuntTop -> Were Hunt Top; UBEBody -> UBE Body.
        // Keep numbers with their series word: Series2, 6th, H2135.
        if (i && upper(text[i]) && (lower(text[i - 1]) || digit(text[i - 1]) ||
            (upper(text[i - 1]) && i + 1 < text.size() && lower(text[i + 1]))))
            result += ' ';
        result += text[i];
    }
    return result;
}

inline OutfitName ParseOutfitName(std::string_view name) {
    using namespace std::literals;
    static constexpr std::array partNames{
        "armor"sv, "armour"sv, "body"sv, "cuirass"sv, "dress"sv, "robe"sv,
        "top"sv, "upper"sv, "shirt"sv, "blouse"sv, "jacket"sv, "coat"sv, "vest"sv,
        "bottom"sv, "lower"sv, "pants"sv, "trousers"sv, "shorts"sv, "skirt"sv,
        "boots"sv, "shoes"sv, "heels"sv, "sabatons"sv,
        "socks"sv, "stocking"sv, "stockings"sv, "tights"sv, "pantyhose"sv, "calf"sv, "thigh"sv,
        "bra"sv, "panty"sv, "panties"sv, "underwear"sv,
        "gloves"sv, "gauntlets"sv, "bracers"sv, "helmet"sv, "helm"sv, "hood"sv, "hat"sv,
        "cape"sv, "cloak"sv, "belt"sv, "mask"sv, "collar"sv, "necktie"sv, "choker"sv,
        "necklace"sv, "earring"sv, "earrings"sv, "bracelet"sv, "ring"sv,
        "acc"sv, "accessory"sv, "accessories"sv
    };
    static constexpr std::array variantNames{"normal"sv, "loose"sv, "slutty"sv, "lewd"sv};
    const auto normalized = LowerAscii(SeparateOutfitWords(name));
    const auto isWord = [](unsigned char ch) {
        return ch >= 128 || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
    };
    std::vector<std::string_view> words;
    for (std::size_t start = 0; start < normalized.size();) {
        if (!isWord(normalized[start])) { ++start; continue; }
        auto end = start + 1;
        while (end < normalized.size() && isWord(normalized[end])) ++end;
        words.emplace_back(normalized.data() + start, end - start);
        start = end;
    }

    OutfitName result;
    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto word = words[i];
        if (word == "high" && i + 1 < words.size() && words[i + 1] == "heels") continue;
        if ((word == "head" || word == "arm") && i + 1 < words.size() &&
            (words[i + 1] == "acc" || words[i + 1] == "accessory" || words[i + 1] == "accessories")) {
            result.part = word == "head" ? "head accessory" : "arm accessory";
            ++i;
            continue;
        }
        if (word == "main" && i + 1 == words.size()) {
            result.part = "main";
            continue;
        }
        const auto part = std::find(partNames.begin(), partNames.end(), word);
        if (part != partNames.end()) {
            result.part = *part;
            if (word == "heels" || word == "shoes" || word == "boots" || word == "sabatons")
                result.part = "footwear";
            if (word == "panty" || word == "panties") result.part = "underwear";
            if (i + 2 == words.size()) {
                const auto variant = std::find(variantNames.begin(), variantNames.end(), words[i + 1]);
                if (variant != variantNames.end()) {
                    result.variant = *variant;
                    ++i;
                }
            }
            continue;
        }
        // A final standalone decimal after a part is a variant, including
        // unknown part names in a multiword outfit ("Bunny Neck 01").
        // Keep embedded numbers, ordinals, and explicit Set/Series numbers.
        if (i > 0 && i + 1 == words.size() &&
            words[i - 1] != "set" && words[i - 1] != "series" &&
            (!result.part.empty() || i >= 2) &&
            std::all_of(word.begin(), word.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
            auto number = word;
            while (number.size() > 1 && number.front() == '0') number.remove_prefix(1);
            result.number = number;
            continue;
        }
        if (!result.family.empty()) result.family += ' ';
        result.family += word;
    }
    return result;
}

// Exact suffix first, unnumbered shared part second, other variants last.
inline int NumberMatch(std::string_view torso, std::string_view part) {
    if (torso == part) return 0;
    return part.empty() ? 1 : 2;
}
enum class VariantMatch { Exact, Shared, Normal, Incompatible };

inline VariantMatch MatchVariant(std::string_view torso, std::string_view part) {
    if (part == torso) return VariantMatch::Exact;
    if (part.empty()) return VariantMatch::Shared;
    if (part == "normal") return VariantMatch::Normal;
    return VariantMatch::Incompatible;
}
inline constexpr std::array<std::string_view, 2> MetalFinishes{" with gold", " with silver"};

inline std::string_view WithoutMetalFinish(std::string_view name) {
    for (const auto suffix : MetalFinishes)
        if (name.ends_with(suffix)) return name.substr(0, name.size() - suffix.size());
    return name;
}

struct FillName {
    std::string_view series;
    std::string_view color;
};

inline FillName ParseFillName(const OutfitName& name) {
    using namespace std::literals;
    static constexpr std::array colors{
        "w r"sv, "black"sv, "blue"sv, "red"sv, "white"sv, "green"sv, "brown"sv,
        "gray"sv, "grey"sv, "purple"sv, "pink"sv, "yellow"sv, "orange"sv, "gold"sv,
        "silver"sv, "beige"sv, "navy"sv, "dark plum"sv, "pale plum"sv, "plum"sv
    };
    static constexpr std::array damage{"damaged"sv, "broken"sv, "torn"sv, "worn"sv};
    const auto hasSuffix = [](std::string_view text, std::string_view word) {
        return text == word || (text.size() > word.size() && text.ends_with(word) &&
            text[text.size() - word.size() - 1] == ' ');
    };
    const auto removeSuffix = [](std::string_view& text, std::string_view word) {
        text.remove_suffix(word.size() + (text.size() > word.size() ? 1 : 0));
    };
    const auto removeDamage = [&](std::string_view& text) {
        const auto word = std::find_if(damage.begin(), damage.end(), [&](auto term) { return hasSuffix(text, term); });
        if (word != damage.end()) removeSuffix(text, *word);
    };
    auto series = std::string_view(name.family);
    removeDamage(series);
    auto palette = WithoutMetalFinish(series);
    if (hasSuffix(palette, "stripe")) removeSuffix(palette, "stripe");
    auto colorBase = palette;
    for (;;) {
        const auto color = std::find_if(colors.begin(), colors.end(),
            [&](auto term) { return hasSuffix(colorBase, term); });
        if (color == colors.end()) break;
        removeSuffix(colorBase, *color);
    }
    if (colorBase.size() == palette.size()) return {series, {}};
    const auto color = palette.substr(colorBase.empty() ? 0 : colorBase.size() + 1);
    removeDamage(colorBase);
    return {colorBase, color};
}

inline std::string_view FillRole(std::string_view part) {
    using namespace std::literals;
    static constexpr std::array lower{"bottom"sv, "lower"sv, "pants"sv, "trousers"sv, "shorts"sv, "skirt"sv};
    return std::find(lower.begin(), lower.end(), part) != lower.end() ? "lower" : part;
}

inline bool HasSeriesPrefix(std::string_view name, std::string_view prefix) {
    return !prefix.empty() && (name == prefix ||
        (name.size() > prefix.size() && name.starts_with(prefix) && name[prefix.size()] == ' '));
}

// Compare whole words, so a series number or part of a word cannot match.
// Allow one shared word within the already selected plugin. Keep explicit
// series-number differences separate even when the preceding word matches.
inline std::string_view CommonSeriesPrefix(std::string_view torso, std::string_view part) {
    std::size_t end{};
    for (std::size_t pos = 0; pos < torso.size();) {
        const auto space = torso.find(' ', pos);
        const auto next = space == std::string_view::npos ? torso.size() : space;
        if (!HasSeriesPrefix(part, torso.substr(0, next))) break;
        end = next;
        pos = next + 1;
    }
    if (!end) return {};
    const auto nextWord = [end](std::string_view name) {
        if (end == name.size()) return std::string_view{};
        name.remove_prefix(end + 1);
        return name.substr(0, name.find(' '));
    };
    const auto left = nextWord(torso);
    const auto right = nextWord(part);
    const auto numbered = [](std::string_view word) {
        return std::any_of(word.begin(), word.end(), [](char ch) { return ch >= '0' && ch <= '9'; });
    };
    if (!left.empty() && !right.empty() && left != right && (numbered(left) || numbered(right))) return {};
    return torso.substr(0, end);
}

// Lower values are preferred. Different explicit colors cannot fill the same set.
inline int FillMatch(FillName torso, FillName part, std::string_view color) {
    if (!part.color.empty() && part.color != color) return -1;
    const bool sameSeries = !torso.series.empty() && torso.series == part.series;
    const bool sameColor = torso.color == part.color;
    if (sameSeries) return sameColor ? 0 : 1;
    return sameColor ? 2 : 3;
}

}
