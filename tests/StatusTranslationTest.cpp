#include "Presets.h"
#include "ExternalTranslation.h"
#include "TranslationKeys.h"
#include <fstream>
#include <map>
#include <iostream>
namespace SKSE::log {
template<class... Args> void info(const char*,Args&&...) {}
template<class... Args> void warn(const char*,Args&&...) {}
}
namespace Gallery {
#include "Localization.inc"
}
int main() {
    using namespace Gallery;
    const std::string original="Outfit restored: My outfit Registered enchantment not restored: My ring";
    language=0;
    if(StatusText(original)!=original) return 1;
    language=1;
    if(StatusText(original)!="装備の復元を確認しました：My outfit 登録した付呪は再現されていません：My ring") return 2;
    language=2;
    externalTranslation=Translation::Parse(R"({"schema":1,"strings":{"Outfit restored: ":"恢复：","Registered enchantment not restored: ":"附魔未恢复："}})",Translation::Keys());
    if(StatusText(original)!="恢复：My outfit 附魔未恢复：My ring") return 3;
    externalTranslation={};
    if(StatusText(original)!=original) return 4;
    const std::string missingSmithing="Registered smithing item is missing; enable Add missing base items to reproduce it";
    language=0;if(StatusText(missingSmithing)!=missingSmithing) return 5;
    language=1;
    if(StatusText(missingSmithing)!="登録した強化装備が見つかりません。再現するには「不足している装備を追加して復元」を有効にしてください。") return 6;
    if(StatusText("Not owned: Fur Boots. Enable Add missing base items for a new game.")!="所持していない装備：Fur Boots。必要ならオプションの「不足している装備を追加して復元」を有効にしてください。") return 7;
    language=2;
    externalTranslation=Translation::Parse(R"({"schema":1,"strings":{"Registered smithing item is missing; enable Add missing base items to reproduce it":"TEST MISSING SMITHING"}})",Translation::Keys());
    if(StatusText(missingSmithing)!="TEST MISSING SMITHING") return 8;
    externalTranslation={};if(StatusText(missingSmithing)!=missingSmithing) return 9;
    const std::string enchantMissing="Cannot reproduce saved enchantment; required plugin is not active: SCBarrels.esp";
    language=0;if(StatusText(enchantMissing)!=enchantMissing)return 10;
    language=1;if(StatusText(enchantMissing)!="付呪を再現できません。必要なプラグインが有効ではありません：SCBarrels.esp")return 11;
    if(StatusText("Saved enchantment reference is missing or changed: Armor.esp / 000800")!="保存した付呪の参照先が見つからないか変更されています：Armor.esp / 000800")return 12;
    if(StatusText("Sample requires a plugin that is not active")!="保存した付呪に必要なプラグインが有効ではありません。")return 13;
    language=2;externalTranslation=Translation::Parse(R"({"schema":1,"strings":{"Cannot reproduce saved enchantment; required plugin is not active: ":"MISSING: "}})",Translation::Keys());
    if(StatusText(enchantMissing)!="MISSING: SCBarrels.esp")return 14;
    externalTranslation={};if(StatusText(enchantMissing)!=enchantMissing)return 15;
    const std::string omitted="Outfit restored: A Skipped unavailable equipment: Ring Saved enchantment omitted: Boots [SCBarrels.esp]";
    language=0;if(StatusText(omitted)!=omitted)return 16;
    language=1;if(StatusText(omitted)!="装備の復元を確認しました：A 見つからない装備を除外しました：Ring 保存した付呪を省いて装備しました：Boots [SCBarrels.esp]")return 17;
    std::cout<<"Enchantment notices: English, Japanese, external and fallback passed\n";
}
