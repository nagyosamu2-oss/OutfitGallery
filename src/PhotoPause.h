#pragma once
namespace Gallery {
// Own only a false -> true transition. Never clear a freeze present before us.
class PhotoPause {
    bool owned{};
public:
    bool Owns() const { return owned; }
    void Toggle(bool& frozen) {
        if(owned) { frozen=false; owned=false; }
        else if(!frozen) { frozen=true; owned=true; }
    }
    void Release(bool& frozen) { if(owned) frozen=false; owned=false; }
};
}
