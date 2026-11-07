#pragma once
#include "Presets.h"
namespace Gallery {
// Appearance state is saved per actor in the existing SKSE co-save section.
void SaveTransmog(SKSE::SerializationInterface*);
bool LoadTransmog(SKSE::SerializationInterface*,std::uint32_t,std::uint32_t,std::uint32_t);
void ResetTransmogPersistence();
void StartTransmogRestore();
bool PlayerTransmogMode();
bool PlayerTransmogAutoOwned();
void RestorePlayerTransmogMode(bool,bool);
bool InstallTransmog();
bool TransmogAvailable();
std::string TransmogUnavailableReason(bool japanese);
bool TransmogActive(std::uint32_t id=0x14);
bool FollowerTransmogMode(std::uint32_t);
void SetFollowerTransmogMode(RE::Actor*,bool);
void SetTransmogAutoOwned(RE::Actor*,bool);
std::vector<std::uint32_t> TransmogAutoOwnedActors();
void ClearActorTransmog(RE::Actor*,bool refresh=true);
void ClearAllTransmog();
bool TransmogMaintenanceDue();
void ApplyTransmog(const Preset&,RE::Actor* actor=nullptr);
bool RemoveEquippedAccessoryAppearance(const Preset&,RE::Actor*);
void ClearTransmog(bool refresh=true);
void BeginTransmogDiagnostic(const Preset&,bool transmog);
void TickTransmogDiagnostic();
}
