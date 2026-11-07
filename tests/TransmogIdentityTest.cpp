#include "TransmogIdentity.h"
#include <iostream>
int main() {
    using namespace Gallery;
    int failures=0;
    // Unrelated body/feet survive head changes; multi-slot wigs are replaced
    // as one armor. A slot53 accessory must not remove a slot32 body.
    if(!RetainAppearanceArmor(1u<<2,2u)) ++failures;
    if(!RetainAppearanceArmor(1u<<7,2u)) ++failures;
    if(RetainAppearanceArmor(2u|(1u<<11),2u)) ++failures;
    if(!RetainAppearanceArmor(1u<<2,1u<<23)) ++failures;
    if(RetainAppearanceArmor(1u<<23,1u<<23)) ++failures;
    if(RetainAppearanceArmor(4u,~0u)) ++failures;
    for(unsigned s=0;s<32;++s) {
        const auto bit=1u<<s;
        if(AppearanceHairMask(bit,0)!=(s==1?0u:bit)) ++failures;
        if(AppearanceHairMask(0,bit)!=(s==1?bit:0u)) ++failures;
    }
    if(AppearanceHairMask(~0u,0)!=(~0u&~2u)) ++failures;
    if(AppearanceHairMask(0,2)!=2) ++failures;
    if(AppearanceHairMask(2,0)!=0) ++failures;
    auto check=[&](unsigned s,std::uint32_t m,bool real,IdentityRestore want) {
        if(ChooseIdentityRestore(s,m,real)!=want) {std::cerr<<"identity policy slot "<<s<<'\n';++failures;}
    };
    for(unsigned s=0;s<32;++s) {
        const auto mask=std::uint32_t{1}<<s;
        const auto excluded=s==9;
        check(s,mask,true,excluded?IdentityRestore::Unchanged:IdentityRestore::Original);
        check(s,mask,false,excluded?IdentityRestore::Unchanged:IdentityRestore::Skin);
        check(s,0,false,excluded?IdentityRestore::Unchanged:IdentityRestore::Original);
        check(s,0,true,excluded?IdentityRestore::Unchanged:IdentityRestore::Original);
    }
    check(32,~0u,false,IdentityRestore::Unchanged);
    check(41,~0u,true,IdentityRestore::Unchanged);
    for(unsigned s=0;s<32;++s) {
        const auto bit=std::uint32_t{1}<<s;
        if(FullLookBodyMask(bit,0)!=(s==9?bit:0u)) ++failures;
        if(FullLookBodyMask(0,bit)!=(s==9?0u:bit)) ++failures;
    }
    // A real slot 53 must not hide skin for an appearance without that slot.
    if(FullLookBodyMask((1u<<23)|(1u<<9),1u<<2)!=((1u<<9)|(1u<<2))) ++failures;
    if(FullLookBodyMask(0,(1u<<23)|(1u<<2))!=((1u<<23)|(1u<<2))) ++failures;
    for(unsigned s=0;s<34;++s) {
        if(ShouldHideRealArmor(s,true,false,false,false)!=(s<32 && s!=9)) ++failures;
        if(ShouldHideRealArmor(s,false,false,false,false)) ++failures;
        if(ShouldHideRealArmor(s,true,true,false,false)) ++failures;
        if(ShouldHideRealArmor(s,true,false,true,false)) ++failures;
        if(ShouldHideRealArmor(s,true,false,false,true)) ++failures;
    }
    for(bool present:{false,true}) for(bool third:{false,true}) for(bool first:{false,true}) {
        if(ShouldStyleBiped(present,third,first,true)!=(present && third && !first)) ++failures;
    }
    if(!ShouldStyleBiped(true,true,true,false)) ++failures;
    if(ShouldStyleBiped(true,false,true,false)) ++failures;
    if(ShouldStyleBiped(false,true,true,false)) ++failures;
    // Preserve female-only gear for women; allow observed male-field fallback;
    // do not admit female-only gear for men or addons with no world model.
    if(!HasAppearanceModel(true,true,false)) ++failures;
    if(!HasAppearanceModel(true,false,true)) ++failures;
    if(!HasAppearanceModel(true,true,true)) ++failures;
    if(HasAppearanceModel(true,false,false)) ++failures;
    if(!HasAppearanceModel(false,true,false)) ++failures;
    if(!HasAppearanceModel(false,true,true)) ++failures;
    if(HasAppearanceModel(false,false,true)) ++failures;
    if(HasAppearanceModel(false,false,false)) ++failures;
    for(bool present:{false,true}) for(bool third:{false,true})
        for(bool first:{false,true}) for(bool player:{false,true}) {
            if(ShouldStylePlayerFirstPerson(present,third,first,player)!=(present && player && first && !third)) ++failures;
        }
    if(FirstPersonAppearanceSlots(~0u)!=0x1Cu) ++failures;
    if(FirstPersonAppearanceSlots((1u<<0)|(1u<<1)|(1u<<7)|(1u<<9)|(1u<<23))) ++failures;
    if(FirstPersonAppearanceSlots((1u<<2)|(1u<<3)|(1u<<4))!=0x1Cu) ++failures;
    return failures?1:0;
}
