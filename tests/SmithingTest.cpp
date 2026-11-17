#include "Smithing.h"
#include <stdexcept>
#include <limits>
int main() {
    using namespace Gallery;
    auto check=[](bool v){if(!v) throw std::runtime_error("Smithing policy regression");};
    check(!ValidSmithing(1.f) && !ValidSmithing(0.f));
    check(ValidSmithing(1.000001f));
    check(!ValidSmithing(std::numeric_limits<float>::infinity()));
    check(!ValidSmithing(std::numeric_limits<float>::quiet_NaN()));
    check(!ProtectPerformance(1.f,1.f,false)); // a generated default-health marker is not smithing
    check(!ProtectPerformance(1.4f,1.4f,false)); // reproduced smithing is reclaimable
    check(ProtectPerformance(1.4f,1.5f,false)); // later smithing is preserved
    check(ProtectPerformance(1.f,1.f,true)); // later custom enchantment is preserved
    check(!ProtectPerformance(1.4f,1.2f,false));
    check(ProtectPerformance(1.f,std::numeric_limits<float>::quiet_NaN(),false));
    check(SelectSmithingCandidate({})==0);
    check(SelectSmithingCandidate({{1,1,false,false}})==0); // an equivalent foreign possession never authorizes reuse
    check(SelectSmithingCandidate({{1,1,true,false},{2,1,false,true}})==1);
    check(SelectSmithingCandidate({{2,1,false,true}})==2);
    check(SelectSmithingCandidate({{1,1,true,false},{2,1,false,true},{3,1,false,true}})==1);
    for(const auto& candidates:std::vector<std::vector<SmithingCandidate>>{
        {{1,1,true,false},{1,1,true,false}},{{2,1,false,true},{3,1,false,true}},{{1,2,true,false}},{{0,1,false,true}}}) {
        bool rejected=false;try {SelectSmithingCandidate(candidates);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected);
    }
    check(IsOwnReclaimEvent(100,0x14,12,100,0x14,0,1,12));
    check(IsOwnReclaimEvent(100,0x14,12,100,0x14,0,1,0)); // engine can omit UID on its deletion notification
    check(!IsOwnReclaimEvent(100,0x14,12,100,0x14,0,1,13));
    check(!IsOwnReclaimEvent(100,0x14,12,100,0x14,123,1,12)); // manual drop/chest transfer still relinquishes claims
    check(!IsOwnReclaimEvent(100,0x14,12,100,0x14,0,2,12));
    check(!IsOwnReclaimEvent(100,0x14,12,101,0x14,0,1,12));
    check(!IsOwnReclaimEvent(100,0x14,12,100,45,0,1,12));
    SmithingCreation creation{100,0x14,12,12};
    creation.Change(100,0x14,12,0x14,12);check(!creation.invalid && creation.current==12 && creation.hops==0);
    creation.Change(101,0x14,12,0x14,13);check(!creation.invalid && creation.current==12);
    creation.Change(100,0x14,12,0x14,13);check(!creation.invalid && creation.current==13 && creation.hops==1);
    creation.Change(100,0x14,13,0x14,14);check(!creation.invalid && creation.current==14);
    creation.Change(100,0x14,99,0x14,15);check(creation.invalid);
    SmithingCreation reported{100,0x14,4,4};
    reported.Change(100,0x14,4,0x14,3);reported.Change(100,0x14,3,0x14,4);
    check(!reported.invalid && reported.current==4 && reported.hops==2);
    SmithingCreation outgoing{100,0x14,12,12};outgoing.Change(100,0x14,12,123,13);check(outgoing.invalid);
    SmithingCreation many{100,0x14,1,1};
    for(std::uint16_t i=1;i<=5;++i) many.Change(100,0x14,i,0x14,i+1);
    check(many.invalid);
    for(float quality:{1.1f,1.2f,1.3712345f}) {
        SmithingGenerationEvidence good;
        good.before=2;good.after=3;good.instanceCount=1;good.matches=1;
        good.actor=0x14;good.owner=0x14;good.expected=14;good.actual=14;
        good.requested=quality;good.actualHealth=quality;
        check(VerifiedSmithingGeneration(good));
        auto reserved=good;reserved.reserved=true;
        check(!VerifiedSmithingGeneration(reserved));
        check(CanRetagSmithingGeneration(reserved,15,false,false));
        check(!CanRetagSmithingGeneration(reserved,15,true,false));
        check(!CanRetagSmithingGeneration(reserved,15,false,true));
        check(!CanRetagSmithingGeneration(reserved,14,false,false));
        auto old=reserved;old.wasPresent=true;check(!CanRetagSmithingGeneration(old,15,false,false));
        old=reserved;old.invalid=true;check(!CanRetagSmithingGeneration(old,15,false,false));
        old=reserved;old.matches=2;check(!CanRetagSmithingGeneration(old,15,false,false));
        old=reserved;old.actualHealth=1.f;check(!CanRetagSmithingGeneration(old,15,false,false));
        old=reserved;old.after=good.before+2;check(!CanRetagSmithingGeneration(old,15,false,false));
        auto bad=good;bad.after=4;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.after=2;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.wasPresent=true;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.reserved=true;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.customEnchanted=true;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.invalid=true;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.actual=15;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.owner=123;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.actualHealth=1.f;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.matches=2;check(!VerifiedSmithingGeneration(bad));
        bad=good;bad.instanceCount=2;check(!VerifiedSmithingGeneration(bad));
    }
    check(ExtraListStorageSize(false)==0x18 && ExtraListStorageSize(true)==0x20);
}
