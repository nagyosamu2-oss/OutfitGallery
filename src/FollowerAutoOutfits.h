#pragma once
namespace Gallery {
void ClearFollowerAutoOutfits();
void SaveFollowerAutoOutfits(SKSE::SerializationInterface*);
bool LoadFollowerAutoOutfit(SKSE::SerializationInterface*,std::uint32_t,std::uint32_t,std::uint32_t);
bool FollowerAutomaticOwns(std::uint32_t); // value-only, thread safe
}
