#pragma once
#include "PreferredItem.h"
#include "RegisteredName.h"
namespace Gallery {
PreferredItem RememberEnchanted(RE::Actor*, RE::ExtraDataList*);
std::uint64_t PreferredScope(std::uint16_t id);
void InitializeManagedItems();
void RegisterManagedItemEvents();
void ResetTransferObservations(); // diagnostic session reset, no inventory mutation
void BeginManagedAddition(RE::Actor*);
void AddManagedItems(RE::Actor*, RE::TESBoundObject*, int count, bool previouslyAbsent, const RegisteredName& name={});
RegisteredName CaptureRegisteredName(RE::TESBoundObject*, RE::InventoryEntryData*);
void ObserveGeneratedItems(RE::Actor*); // game thread, once after additions, before equip
void TrackNewManagedItems(RE::Actor*, bool compatibility=false);
void ReclaimManagedItems(RE::Actor*); // game thread, only after a verified successful change
}
