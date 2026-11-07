#pragma once
#include <cstdint>
namespace Gallery {
// First-person arm geometry lives in torso, hand and forearm objects. Shields
// retain the real equipment path; head/feet/accessories stay third-person only.
inline constexpr std::uint32_t FirstPersonAppearanceMask=(1u<<2)|(1u<<3)|(1u<<4);
constexpr bool ShouldStylePlayerFirstPerson(bool present,bool thirdPerson,bool firstPerson,bool isPlayer) {
    return present && isPlayer && firstPerson && !thirdPerson;
}
constexpr std::uint32_t FirstPersonAppearanceSlots(std::uint32_t slots) {
    return slots & FirstPersonAppearanceMask;
}
// A female actor can render a male-field mesh when its female model is absent.
// Observed on Valmilia wearing BDOR Cernay in the normal equipment pipeline.
// This is availability only: leave actual selection and loading to the engine.
constexpr bool HasAppearanceModel(bool isFemale,bool maleModel,bool femaleModel) {
    return maleModel || (isFemale && femaleModel);
}
constexpr bool RetainAppearanceArmor(std::uint32_t armor,std::uint32_t replacement) {
    return (armor&replacement)==0;
}
constexpr std::uint32_t AppearanceHairMask(std::uint32_t real,std::uint32_t appearance) {
    constexpr auto hair=1u<<1; // editor slot31, not long-hair collision slot41
    return (real&~hair)|(appearance&hair);
}
constexpr std::uint32_t FullLookBodyMask(std::uint32_t real,std::uint32_t appearance) {
    // The shield remains real; all clothing coverage comes from the selected
    // armor records, independent of asynchronous addon staging.
    return (real&(1u<<9)) | (appearance&~(1u<<9));
}
constexpr bool ShouldHideRealArmor(unsigned slot,bool armor,bool shield,bool skin,bool style) {
    return slot<32 && slot!=9 && armor && !shield && !skin && !style;
}
// Only a positively identified third-person holder is eligible. Unknown and
// first-person passes must retain the original engine results.
constexpr bool ShouldStyleBiped(bool present,bool thirdPerson,bool firstPerson, bool isPlayer) {
    return present && thirdPerson && !(isPlayer && firstPerson);
}

enum class IdentityRestore { Unchanged, Original, Skin };
constexpr IdentityRestore ChooseIdentityRestore(unsigned slot, std::uint32_t staged, bool originalPresent) {
    // Slot 39 is shared with an off-hand weapon; never replace its identity.
    if(slot>=32 || slot==9) return IdentityRestore::Unchanged;
    if(originalPresent || !(staged&(std::uint32_t{1}<<slot))) return IdentityRestore::Original;
    // A newly staged model needs a non-null owner for deferred attachment.
    // Skin represents no worn equipment without lending the style's armor type.
    return IdentityRestore::Skin;
}
}
