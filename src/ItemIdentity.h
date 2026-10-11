#pragma once
#include "Presets.h"
#include "FormIdentityPolicy.h"
namespace Gallery {
inline const RE::TESFile* ItemOwner(RE::TESForm* form) {
 auto* data=RE::TESDataHandler::GetSingleton();
 const auto* owner=form?FormOwner(data,form->GetFormID()):nullptr;
 if(!owner || data->LookupForm(LocalFormID(form->GetFormID()),owner->GetFilename())!=form)
  throw std::runtime_error("Cannot save an unstable item reference");
 return owner;
}
inline RE::TESForm* ResolveItem(const Item& item) {
 auto* data=RE::TESDataHandler::GetSingleton();
 if(!data) return nullptr;
 auto matchesKind=[&](RE::TESForm* f) {
  return f && ((item.kind=="armor" && f->As<RE::TESObjectARMO>()) ||
   (item.kind=="weapon" && f->As<RE::TESObjectWEAP>()) || (item.kind=="ammo" && f->As<RE::TESAmmo>()));
 };
 auto* direct=data->LookupForm(item.localID,item.plugin);
 if(matchesKind(direct)) return direct;
 if(item.kind!="armor" || item.name.empty()) return nullptr;
 const auto* source=data->LookupModByName(item.plugin);
 return ResolveLegacyInjected<RE::TESForm>(nullptr,source,item.localID,
  [&](const RE::TESFile* master,std::uint32_t id){return data->LookupForm(id,master->GetFilename());},
  [&](RE::TESForm* candidate,const RE::TESFile* file){
   return matchesKind(candidate) && candidate->GetFile(0)==file &&
    candidate->GetName() && item.name==candidate->GetName();
  });
}
}
