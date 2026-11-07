#include "AccessoryToggle.h"
#include "TransmogSave.h"
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void require(bool v){if(!v) throw std::runtime_error("Accessory toggle regression");}
int main(){try{
    using namespace AccessoryToggle;
    require(!Allowed(false,0,false)); // full outfit cannot remove by default
    require(Allowed(false,0,true));
    require(Allowed(true,0,false));
    require(Allowed(false,0x1803,false));
    require(!Complete({},{}));
    require(!Complete({11,12},{11,99}));
    require(!Complete({11,12},{11}));
    require(Complete({11,12},{90,11,12,91}));
    require(Remaining({90,11,12,91},{11,12})==std::vector<std::uint32_t>({90,91}));
    require(Complete({11},{11,12}));
    require(Remaining({11,12},{11})==std::vector<std::uint32_t>({12}));
    require(!Complete({0},{0}));
    require(!Complete({11},{21}));
    require(Remaining({11},{11}).empty());
    // Last visual accessory removed: keep controlled scope over save/load.
    TransmogSave::State empty{0xFF001216,0x13746,0x1000,true,false,true,{}};
    require(TransmogSave::Decode(TransmogSave::Encode(empty))==empty);
    unsigned calls{};
    require(TransmogSave::Remap(empty,[&](auto id,auto& now){++calls;now=id+1;return true;}));
    require(calls==2 && empty.race==0x13747 && empty.scope==0x1000 && empty.armor.empty());
    require(TransmogSave::Remap(empty,[&](auto id,auto& now){now=id;return id!=0x13747;}));
    require(!empty.race && !empty.scope && empty.mode);
    std::cout<<"PASS: complete/partial/replaced sets, exact removal, last-item appearance persistence\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
