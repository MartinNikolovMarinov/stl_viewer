#pragma once

#include <fwd_internal.h>

namespace stlv {

struct Timer {
    f64 start;
    f64 delta;
};

void clockClear(Timer& clock);
void clockStart(Timer& clock, f64 time);
void clockUpdate(Timer& clock, f64 time);

} // namespace stlv
