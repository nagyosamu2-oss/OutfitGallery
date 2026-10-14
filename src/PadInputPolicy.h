#pragma once
#include <array>
#include <cstdint>
namespace PadInputPolicy {
inline unsigned SelectSource(unsigned raw,unsigned translated,unsigned mapped) {
    return (raw & ~mapped) | (translated & mapped);
}
// Ignore the opening button on the release frame too: UI translation can lag
// the physical release and still report an edge during that frame.
inline unsigned RemainingOpeningBlock(unsigned blocked,unsigned physicalHeld,unsigned translatedHeld) {
    return blocked & (physicalHeld | translatedHeld);
}
inline bool MayReleaseFocus(unsigned fresh,bool modal,bool binding,bool mouseDown) {
    return fresh && !modal && !binding && !mouseDown;
}
struct RepeatState {
    std::array<std::uint64_t,18> next{};
    void Reset() {next.fill(0);}
    unsigned Update(unsigned edges,unsigned held,std::uint64_t now) {
        unsigned repeat=edges;
        for(unsigned n=0;n<18;++n) {
            const auto bit=1u<<n;
            if(edges&bit) next[n]=now+350;
            else if((held&bit) && now>=next[n]) {repeat|=bit;next[n]=now+100;}
            if(!(held&bit)) next[n]=now+350;
        }
        return repeat;
    }
};
}
