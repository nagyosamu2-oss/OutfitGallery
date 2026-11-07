#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace Gallery::AccessoryToggle {
inline bool Allowed(bool accessories,std::uint32_t headScope,bool fullEnabled) {return accessories || headScope || fullEnabled;}
inline bool Complete(const std::vector<std::uint32_t>& wanted,const std::vector<std::uint32_t>& current) {
    return !wanted.empty() && std::all_of(wanted.begin(),wanted.end(),[&](auto id) {
        return id && std::find(current.begin(),current.end(),id)!=current.end();
    });
}
inline std::vector<std::uint32_t> Remaining(const std::vector<std::uint32_t>& current,const std::vector<std::uint32_t>& wanted) {
    auto result=current;
    std::erase_if(result,[&](auto id){return std::find(wanted.begin(),wanted.end(),id)!=wanted.end();});
    return result;
}
}
