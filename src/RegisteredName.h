#pragma once
#include <string>
#include <string_view>
namespace Gallery {
// Applies only after identifying a tracked generated instance; never grants ownership.
inline bool ProtectGeneratedItem(bool compatibility,bool quest,bool tempered,bool enchanted,
    bool named,bool favorite,bool poisoned) {
    return quest || tempered || enchanted || favorite || poisoned || (named && !compatibility);
}
struct RegisteredName {
    std::string base, display;
    bool operator==(const RegisteredName&) const = default;
};
inline bool ValidRegisteredName(const RegisteredName& n) {
    return !n.base.empty() && !n.display.empty() && n.base.size()<=512 && n.display.size()<=512 &&
        n.base.find('\0')==std::string::npos && n.display.find('\0')==std::string::npos;
}
// Matching text is only a name-protection exception, never an ownership claim.
inline bool MatchesRegisteredName(bool enabled,bool tracked,const RegisteredName& expected,
    std::string_view base,std::string_view display,bool simpleName) {
    return enabled && tracked && simpleName && ValidRegisteredName(expected) &&
        expected.base==base && expected.display==display;
}
}
