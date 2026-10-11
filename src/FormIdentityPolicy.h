#pragma once
#include <cstdint>
namespace Gallery {
template<class Handler>
auto FormOwner(Handler* data,std::uint32_t id) {
 using Result=decltype(data->LookupLoadedModByIndex(0));
 if(!data || (id>>24)==0xFF) return Result{};
 return (id>>24)==0xFE ? data->LookupLoadedLightModByIndex((id>>12)&0xFFF) : data->LookupLoadedModByIndex(id>>24);
}
constexpr std::uint32_t LocalFormID(std::uint32_t id) {return id & ((id>>24)==0xFE?0xFFFu:0xFFFFFFu);}
template<class Form,class File,class Lookup,class Match>
Form* ResolveLegacyInjected(Form* direct,const File* source,std::uint32_t local,Lookup lookup,Match match) {
 if(direct) return direct;
 if(!source || source->compileIndex==0xFF) return nullptr;
 Form* found=nullptr;
 for(std::uint32_t n=0;n<source->masterCount;++n) {
  const auto* master=source->masterPtrs[n];
  if(!master || master->compileIndex==0xFF) continue;
  auto* candidate=lookup(master,local);
  if(!candidate || !match(candidate,source)) continue;
  if(found && found!=candidate) return nullptr; // Never guess among matches.
  found=candidate;
 }
 return found;
}
}
