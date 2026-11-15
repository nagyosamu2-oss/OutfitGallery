#include "TransmogHead.h"
#include <cassert>
#include <initializer_list>
int main() {
    using namespace Gallery;
    constexpr auto head=1u, hair=2u, body=4u, circlet=1u<<12, ears=1u<<13;
    constexpr auto helm=head|hair|circlet|ears;
    // Full-face real helmet, bare appearance: show the head, keep unrelated bits.
    assert(AppearanceHeadMask(helm|body,body,~0u,helm)==(circlet|ears|body));
    // Appearance helmet and wig must retain their intended head/hair hiding.
    assert((AppearanceHeadMask(0,helm,~0u,0)&3u)==3u);
    assert((AppearanceHeadMask(helm,hair,~0u,helm)&3u)==hair);
    // An accessory unrelated to headgear must leave the real helmet intact.
    assert(HiddenHeadCoverage(helm,helm,body)==0);
    assert(AppearanceHeadMask(helm,body,body,0)==helm);
    // Head-only replacement removes the entire intersecting real helmet.
    assert(HiddenHeadCoverage(helm,helm,hair)==helm);
    assert((AppearanceHeadMask(helm,hair,hair,helm)&3u)==hair);
    assert(HeadPartitionsToRestore(helm,0)==(head|ears));
    assert(HeadPartitionsToRestore(helm,helm)==0);
    assert(HeadPartitionsToRestore(helm,ears)==head);
    assert(HeadPartitionsToRestore(0,0)==0);
    assert(HiddenHeadCoverage(helm|(1u<<9),helm,~0u)==0);
    assert(HeadPartitionBit(30)==head && HeadPartitionBit(130)==head);
    assert(HeadPartitionBit(43)==ears && HeadPartitionBit(143)==ears);
    for(unsigned section:{0u,31u,32u,50u,51u,101u,131u,150u,230u,65535u})
        assert(HeadPartitionBit(section)==0);
    for(unsigned bit=0;bit<32;++bit) {
        const auto flag=1u<<bit;
        assert((HeadPartitionsToRestore(~0u,0)&flag)==(HeadPartitionBits&flag));
        if(bit>1) assert(AppearanceHeadMask(flag,~0u,~0u,~0u)&flag);
    }
}
