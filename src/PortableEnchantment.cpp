#include "PortableEnchantment.h"
#include "EnchantmentRecipe.h"
#include "EnchantmentSignature.h"
#include "ActivePluginLookup.h"
#include <memory>
namespace Gallery {
namespace {using EnchantProbe::Json;
Json FormKey(RE::TESForm* form) {
 if(!form)return nullptr;
 auto* file=form->GetFile(0);
 EnchantProbe::Require(file && (form->GetFormID()>>24)!=0xFF,"Cannot save a dynamic form reference");
 return Json::array({std::string(file->GetFilename()),form->GetFormID()&(file->IsLight()?0xFFFu:0xFFFFFFu)});
}
RE::TESForm* Resolve(const Json& key) {
 EnchantProbe::Key(key);
 auto* data=RE::TESDataHandler::GetSingleton();auto name=key[0].get<std::string>();
 auto* file=FindActivePlugin(data,name);
 EnchantProbe::Require(file!=nullptr,"Sample requires a plugin that is not active");
 auto id=EnchantProbe::UInt(key[1]);
 EnchantProbe::Require(!file->IsLight() || id<=0xFFF,"Invalid light-plugin local ID");
 auto* form=data->LookupForm(id,name);
 EnchantProbe::Require(form && FormKey(form)==key,"Portable reference did not resolve exactly");return form;
}
Json Encode(RE::EnchantmentItem* enchant,std::uint16_t charge,bool temporary,float health) {
 EnchantProbe::Require(enchant && !enchant->effects.empty() && enchant->effects.size()<=EnchantProbe::MaxEffects,"Enchantment reproduction supports 1 to 32 effects");
 Json effects=Json::array();
 for(const auto* effect:enchant->effects) {
  EnchantProbe::Require(effect && effect->baseEffect && !effect->conditions.head,"Conditional or missing effect unsupported");
  effects.push_back(Json::array({FormKey(effect->baseEffect),std::bit_cast<std::uint32_t>(effect->effectItem.magnitude),effect->effectItem.area,effect->effectItem.duration,std::bit_cast<std::uint32_t>(effect->cost)}));
 }
 const auto& d=enchant->data;
 return Json::array({1,effects,d.costOverride,d.flags.underlying(),static_cast<int>(d.castingType),d.chargeOverride,static_cast<int>(d.delivery),static_cast<int>(d.spellType),std::bit_cast<std::uint32_t>(d.chargeTime),FormKey(d.baseEnchantment),FormKey(d.wornRestrictions),charge,temporary,std::bit_cast<std::uint32_t>(health)});
}
}
std::string CapturePortableEnchantment(RE::ExtraDataList* extra) {
 if(!extra || !extra->HasType<RE::ExtraEnchantment>())return {};
 auto text=EnchantmentSignature(extra);auto j=EnchantProbe::ParseSignature(text);return j.dump();
}
void AttachPortableEnchantment(RE::ExtraDataList* extra,const std::string& text) {
 EnchantProbe::Require(extra && !extra->HasType<RE::ExtraEnchantment>(),"Fresh enchantment list required");
 const auto signature=EnchantProbe::ParseSignature(text);
 for(int n:{9,10})if(!signature[n].is_null())Resolve(signature[n]);
 RE::BSTArray<RE::Effect> effects;
 for(const auto& item:signature[1]) {
  auto* base=Resolve(item[0])->As<RE::EffectSetting>();EnchantProbe::Require(base!=nullptr,"Sample effect is not a magic effect");
  RE::Effect effect;effect.baseEffect=base;effect.effectItem.magnitude=EnchantProbe::FloatBits(item[1]);
  effect.effectItem.area=EnchantProbe::UInt(item[2]);effect.effectItem.duration=EnchantProbe::UInt(item[3]);effect.cost=EnchantProbe::FloatBits(item[4]);effects.push_back(effect);
 }
 auto* manager=RE::BGSCreatedObjectManager::GetSingleton();EnchantProbe::Require(manager!=nullptr,"Created-object manager unavailable");
 // Called only for a missing registered recipe in the existing equipment lane.

 auto* enchant=manager->AddArmorEnchantment(effects);EnchantProbe::Require(enchant!=nullptr,"Game could not create the armor enchantment");
 auto releaseFactory=[manager](RE::EnchantmentItem* value){if(value)manager->DestroyEnchantment(value,false);};
 std::unique_ptr<RE::EnchantmentItem,decltype(releaseFactory)> factoryReference(enchant,releaseFactory);
 const auto health=EnchantProbe::FloatBits(signature[13]);const auto charge=static_cast<std::uint16_t>(EnchantProbe::UInt(signature[11],65535));
 const auto recreated=Encode(enchant,charge,false,health);
 const bool equivalent=EnchantProbe::FactoryEquivalent(text,recreated.dump());
 SKSE::log::info("OG-ENCHANT factory form={:08X} expected={} actual={} exact={} performanceEquivalent={}",enchant->GetFormID(),signature.dump(),recreated.dump(),signature==recreated,equivalent);
 if(!equivalent) {
  throw std::runtime_error("Factory metadata differs; no inventory item added. See OutfitGallery.log");
 }
 // Transfer the factory reference into a new extra list, following SKSE's
 // CreateEnchantment path. Existing possessions and base records are never edited.
 extra->Add(new RE::ExtraEnchantment(enchant,charge,false));
 factoryReference.release(); // the fresh list now carries the factory reference

}
}
