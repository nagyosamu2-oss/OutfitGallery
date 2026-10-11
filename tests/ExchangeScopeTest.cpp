#include "Presets.h"
#include <iostream>
#include <stdexcept>
int main() {
    using namespace Gallery;
    try {
        // Reversed overlapping masks require transitive closure; unrelated bit 8 survives.
        const std::vector<std::uint32_t> connected{6,3,8};
        if(ToggleExchangeSlot(8,1,true,connected)!=15) throw std::runtime_error("linked slots not selected together");
        if(ToggleExchangeSlot(15,1,false,connected)!=8) throw std::runtime_error("linked slots not cleared together");
        if(ToggleExchangeSlot(0,0x80000000u,true,{0x80000000u})!=0x80000000u) throw std::runtime_error("high slot lost");
        if(!CrossesExchangeBoundary(6,2) || CrossesExchangeBoundary(2,6) || CrossesExchangeBoundary(8,6)) throw std::runtime_error("worn conflict classification incorrect");
        // Reported regression: incoming hair=31, old wig=31+42, earring=43.
        // Hair OFF preserves the wig; ON replaces the entire wig, without
        // expanding selection to 42 and accidentally touching a separate crown.
        constexpr std::uint32_t hair=1u<<1, crown=1u<<12, ear=1u<<13;
        if(ReplaceWornForScope(hair|crown,ear)) throw std::runtime_error("hair OFF removed wig");
        if(!ReplaceWornForScope(hair|crown,hair|ear)) throw std::runtime_error("hair ON failed to replace multi-slot wig");
        if(ReplaceWornForScope(crown,hair|ear)) throw std::runtime_error("scope expanded into unselected crown slot");
        if(ReplaceWornForScope(hair|crown,0)) throw std::runtime_error("empty selection removed equipment");
        Preset p{"shared","shared.png",{{"Test.esp",1,"a","armor",""},{"Test.esp",2,"b","armor",""},{"Test.esp",3,"c","armor",""}},10,true};
        p.exchangeSlots=2;
        auto result=FilterExchangeSlots(p,{2,2,8});
        if(result.items.size()!=2 || result.items[0].name!="a" || result.items[1].name!="b" || result.slotMask!=2) throw std::runtime_error("shared-slot selection inconsistent");
        bool caught=false;try{FilterExchangeSlots(p,{2});}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("incomplete resolution accepted");
        p.slotMask=0; p.accessories=false;
        p.items[0].preferred={true,123,5,"enchantment"};
        if(AvailableExchangeSlots(p,{2,2,8})!=10) throw std::runtime_error("full outfit slots missing");
        result=FilterExchangeSlots(p,{2,2,8});
        if(result.slotMask!=0 || result.accessories || result.items.size()!=2 || result.items[0].preferred.signature!="enchantment" || p.slotMask || p.items.size()!=3)
            throw std::runtime_error("full outfit subset changed original or enchantment");
        if(ReplaceWornForScope(8,result.slotMask)) throw std::runtime_error("unselected armor replaced");
        p.exchangeSlots=10; result=FilterExchangeSlots(p,{2,2,8});
        if(result.slotMask!=0 || result.items.size()!=3) throw std::runtime_error("select all reverted to full replacement");
        p.exchangeSlots=0; result=FilterExchangeSlots(p,{2,2,8});
        if(!result.items.empty() || !result.exchangeSlots || *result.exchangeSlots) throw std::runtime_error("full outfit clear is not explicit no-op");
        p.exchangeSlots=4; caught=false;try{FilterExchangeSlots(p,{2,2,8});}catch(const std::exception&){caught=true;}
        if(!caught) throw std::runtime_error("full outfit accepted unsaved slot");
        p.exchangeSlots.reset(); result=FilterExchangeSlots(p,{});
        if(result.slotMask || result.exchangeSlots || result.items.size()!=3) throw std::runtime_error("restore original full outfit failed");
        p.items.push_back({"Test.esp",4,"old weapon","weapon","right"});
        p.exchangeSlots=10; result=FilterExchangeSlots(p,{2,2,8,0});
        if(result.items.size()!=3) throw std::runtime_error("legacy weapon entered armor selection");
        if(AvailableExchangeSlots(p,{2,2,8,0x80000000u})!=10) throw std::runtime_error("legacy weapon contributed slots");
        p.outfitItems=std::vector<std::string>{ExchangeItemKey(p.items[1])};
        result=FilterExchangeSlots(p,{});
        if(result.items.size()!=1 || result.items[0].localID!=2 || result.slotMask || result.accessories) throw std::runtime_error("independent item sharing slot failed");
        p.outfitItems.reset();p.items.back().kind="armor";
        p.exchangeSlots=0;p.exchangeSlotless={ExchangeItemKey(p.items.back())};
        result=FilterExchangeSlots(p,{2,2,8,0});
        if(result.slotMask || result.accessories || result.items.size()!=1) throw std::runtime_error("full slotless selection became partial");
        std::cout<<"PASS: linked slots, shared slots, high bit, conflict boundaries, invalid scope resolution\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
