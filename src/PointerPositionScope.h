#pragma once
#include <limits>
// Only this callback sees an invalid pointer. Other users of the shared ImGui IO
// receive the original values even on an early return.
template<class IO> struct PointerPositionScope {
    IO& io;
    decltype(io.MousePos) pos;
    decltype(io.MouseDelta) delta;
    PointerPositionScope(IO& value,bool hidden):io(value),pos(value.MousePos),delta(value.MouseDelta) {
        if(hidden) {io.MousePos={-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};io.MouseDelta={0,0};}
    }
    ~PointerPositionScope() {io.MousePos=pos;io.MouseDelta=delta;}
    PointerPositionScope(const PointerPositionScope&)=delete;
};
