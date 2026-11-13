#include "RandomTryOnNames.h"
#include "TryOnCustomization.h"
#include <cassert>

int main() {
    using namespace Gallery::RandomTryOn;
    for (const auto* style : {"Gothic", "Casual", "Tactical"}) {
        const auto shorts = ParseOutfitName(std::string(style) + " Volkihar Shorts");
        const auto top = ParseOutfitName(std::string(style) + " Volkihar Crop Top");
        const auto other = ParseOutfitName(std::string(style == std::string_view("Casual") ? "Gothic" : "Casual") + " Volkihar Crop Top");
        const auto shared = CommonSeriesPrefix(ParseFillName(shorts).series, ParseFillName(top).series);
        assert(shared == shorts.family);
        assert(HasSeriesPrefix(ParseFillName(top).series, shared));
        assert(!HasSeriesPrefix(ParseFillName(other).series, shared));
    }
    const std::vector<std::uint32_t> customSlots{1, 2, 4, 3};
    auto custom = CustomizeSelection(customSlots, {true,true,true,false}, 3, true);
    assert(!custom[0] && !custom[1] && custom[2] && custom[3]);
    custom = CustomizeSelection(customSlots, custom, 3, false);
    assert(!custom[0] && !custom[1] && custom[2] && !custom[3]);
    custom = CustomizeSelection(customSlots, custom, 0, true);
    assert(custom[0] && custom[2]);
    custom = CustomizeSelection(customSlots, custom, 0, false);
    custom = CustomizeSelection(customSlots, custom, 2, false);
    assert(std::none_of(custom.begin(), custom.end(), [](bool chosen) {return chosen;}));
    bool invalid = false;
    try { CustomizeSelection(customSlots, custom, 4, true); } catch (const std::runtime_error&) {invalid=true;}
    assert(invalid);
    assert(SeparateOutfitWords("WereHuntTop") == "Were Hunt Top");
    assert(SeparateOutfitWords("UBEBody") == "UBE Body");
    assert(SeparateOutfitWords("WereHunt FUR") == "Were Hunt FUR");
    assert(SeparateOutfitWords("Series2Top") == "Series2 Top");
    assert(SeparateOutfitWords("H2135 6th") == "H2135 6th");
    const auto were = ParseOutfitName("WereHuntTop");
    assert(were.family == "were hunt");
    for (const auto* text : {"WereHuntBoots", "WereHuntJacket", "WereHuntPants",
                             "WereHuntBelt", "WereHuntNeck", "WereHunt FUR"}) {
        const auto part = ParseOutfitName(text);
        assert(CommonSeriesPrefix(ParseFillName(were).series, ParseFillName(part).series) == "were hunt");
    }
    const auto were2 = ParseOutfitName("WereHunt2Top");
    const auto were3 = ParseOutfitName("WereHunt3Pants");
    assert(CommonSeriesPrefix(were2.family, were3.family).empty());
    const auto doll = ParseOutfitName("Broken Doll Body");
    for (const auto* text : {"Broken Doll Feet", "Broken Doll Hand", "Broken Doll St",
                             "Broken Doll Neck", "Broken Doll Leotard", "Broken Doll Hat"}) {
        const auto part = ParseOutfitName(text);
        assert(CommonSeriesPrefix(ParseFillName(doll).series, ParseFillName(part).series) == "broken doll");
    }
    assert(CommonSeriesPrefix("broken doll", "broken sword") == "broken");
    const auto barbarian = ParseOutfitName("Barbarian Harness");
    const auto barbarianSkirt = ParseOutfitName("Barbarian Skirt");
    assert(CommonSeriesPrefix(barbarian.family, barbarianSkirt.family) == "barbarian");
    assert(CommonSeriesPrefix("barbarian harness", "barbarian greaves") == "barbarian");
    assert(CommonSeriesPrefix("barbarian harness", "warrior skirt").empty());
    assert(!HasSeriesPrefix("barbarians", "barbarian"));
    assert(CommonSeriesPrefix("collection set2 harness", "collection set20 skirt").empty());
    assert(CommonSeriesPrefix("gunslinger 6th", "gunslinger 7th").empty());
    assert(CommonSeriesPrefix("collection set2", "collection set20").empty());
    assert(CommonSeriesPrefix("masquerade", "masquerade unknownpiece") == "masquerade");
    assert(CommonSeriesPrefix("broken doll special", "broken doll neck") == "broken doll");
    assert(HasSeriesPrefix("broken doll st", "broken doll"));
    assert(!HasSeriesPrefix("broken dolly", "broken doll"));
    assert(!HasSeriesPrefix("anything", ""));
    const auto mistTop = ParseOutfitName("Dark Mist Top - Red & Pale Plum");
    const auto mistPants = ParseOutfitName("Dark Mist Pants - Red & Pale Plum with Silver");
    const auto mistOtherPants = ParseOutfitName("Dark Mist Pants - Red & Dark Plum with Gold");
    const auto mistSkirt = ParseOutfitName("Dark Mist Skirt - Plum");
    const auto mist = ParseFillName(mistTop);
    assert(mist.series == "dark mist" && mist.color == "red pale plum");
    assert(WithoutMetalFinish(mistPants.family) == mistTop.family);
    assert(WithoutMetalFinish(mistOtherPants.family) != mistTop.family);
    assert(FillMatch(mist, ParseFillName(mistPants), mist.color) == 0);
    assert(FillMatch(mist, ParseFillName(mistOtherPants), mist.color) < 0);
    assert(FillMatch(mist, ParseFillName(mistSkirt), mist.color) < 0);
    const auto stripeName = ParseOutfitName("H2135's P.Series2 Top Black Stripe");
    const auto blackPants = ParseOutfitName("H2135's P.Series2 Pants [Black]");
    const auto damagedPants = ParseOutfitName("H2135's P.Series2 Pants [Black] Damaged");
    const auto bluePants = ParseOutfitName("H2135's P.Series2 Pants [Blue]");
    const auto blackSkirt = ParseOutfitName("H2135's P.Series2 Skirt [Black]");
    const auto plainPants = ParseOutfitName("H2135's P.Series2 Pants");
    const auto stripe = ParseFillName(stripeName);
    assert(stripe.color == "black");
    assert(FillMatch(stripe, ParseFillName(blackPants), stripe.color) == 0);
    assert(FillMatch(stripe, ParseFillName(damagedPants), stripe.color) == 0);
    assert(FillMatch(stripe, ParseFillName(plainPants), stripe.color) > 0);
    assert(FillMatch(stripe, ParseFillName(bluePants), stripe.color) < 0);
    assert(FillRole(blackPants.part) == FillRole(blackSkirt.part));
    for (const auto* name : {"Gunslinger 6th Main", "Gunslinger 6th Boots", "Gunslinger 6th Coat"})
        assert(ParseOutfitName(name).family == "gunslinger 6th");
    assert(ParseOutfitName("Main Street Boots").family == "main street");
    assert(ParseOutfitName("Gunslinger 7th Coat").family != ParseOutfitName("Gunslinger 6th Main").family);
    for (const auto* name : {"Masquerade Jacket", "Masquerade Pants", "Masquerade Shoes",
                             "Masquerade Mask", "Masquerade Collar"})
        assert(ParseOutfitName(name).family == "masquerade");

    for (const auto* name : {"Pure White Upper", "Pure White Upper (Bra)", "Pure White Necktie",
                             "Pure White Choker", "Pure White Necklace", "Pure White Calf",
                             "Pure White Thigh", "Pure White High Heels 2", "Pure White Underwear 3"})
        assert(ParseOutfitName(name).family == "pure white");

    assert(ParseOutfitName("Pure White Underwear 1").part == ParseOutfitName("Pure White Underwear 3").part);
    assert(ParseOutfitName("Pure White Heels").part == ParseOutfitName("Pure White High Heels 2").part);
    assert(ParseOutfitName("Pure White Jacket 2").family == ParseOutfitName("Pure White Jacket 1").family);
    assert(ParseOutfitName("Pure White Jacket 2").number != ParseOutfitName("Pure White Jacket 1").number);
    const auto bunny = ParseOutfitName("VRC Succubus Bunny Body 01");
    assert(bunny.family == "vrc succubus bunny" && bunny.number == "1");
    for (const auto* name : {"VRC Succubus Bunny Ear 01", "VRC Succubus Bunny Feet 01",
                            "VRC Succubus Bunny Neck 01", "VRC Succubus Bunny St 01", "VRC Succubus Bunny Tail 01"}) {
        const auto piece = ParseOutfitName(name);
        assert(CommonSeriesPrefix(bunny.family, piece.family) == bunny.family);
        assert(NumberMatch(bunny.number, piece.number) == 0);
        auto alternatives = std::vector{ParseOutfitName("VRC Succubus Bunny Neck 03"),
            ParseOutfitName("VRC Succubus Bunny Neck"), piece};
        std::stable_sort(alternatives.begin(), alternatives.end(), [&](const auto& a, const auto& b) {
            return NumberMatch(bunny.number, a.number) < NumberMatch(bunny.number, b.number);
        });
        assert(alternatives.front().number == "1");
    }
    assert(NumberMatch("1", "1") < NumberMatch("1", ""));
    assert(NumberMatch("1", "") < NumberMatch("1", "2"));
    assert(ParseOutfitName("Gunslinger 6th Main").number.empty());
    assert(ParseOutfitName("Collection Set 2").family == "collection set 2");
    assert(ParseOutfitName("Collection Series 20").number.empty());
    assert(ParseOutfitName("Gunslinger 6").family == "gunslinger 6");
    assert(ParseOutfitName("Pure Black Stockings").family != ParseOutfitName("Pure White Upper").family);
    assert(ParseOutfitName("Masquerade 2 Jacket").family != ParseOutfitName("Masquerade 3 Pants").family);
    using Gallery::RandomTryOn::MatchVariant;
    using Gallery::RandomTryOn::VariantMatch;
    const auto loose = ParseOutfitName("Osmanthus Wine Dress Loose");
    const auto boots = ParseOutfitName("Osmanthus Wine Boots Normal");
    const auto panty = ParseOutfitName("Osmanthus Wine Panty Normal");
    const auto arm = ParseOutfitName("Osmanthus Wine Arm Acc");
    const auto head = ParseOutfitName("Osmanthus Wine Head Acc");
    for (const auto* part : {&loose, &boots, &panty, &arm, &head})
        assert(part->family == "osmanthus wine");
    assert(loose.variant == "loose");
    assert(panty.part == "underwear");
    assert(arm.part != head.part);
    assert(MatchVariant(loose.variant, boots.variant) == VariantMatch::Normal);
    assert(MatchVariant(loose.variant, panty.variant) == VariantMatch::Normal);
    assert(MatchVariant(loose.variant, arm.variant) == VariantMatch::Shared);
    assert(MatchVariant("loose", "loose") < MatchVariant("loose", "normal"));
    for (const auto* name : {"Osmanthus Wine Panty Slutty", "Osmanthus Wine Panty Lewd"}) {
        const auto variant = ParseOutfitName(name);
        assert(variant.family == loose.family);
        assert(MatchVariant(loose.variant, variant.variant) == VariantMatch::Incompatible);
    }
    assert(ParseOutfitName("Osmanthus Blue Dress Loose").family != loose.family);
    assert(Gallery::RandomTryOn::LowerAscii("Obi's Masquerade UBE patch.esp").find("ube") != std::string::npos);
}
