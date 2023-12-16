#pragma once

#include <fwd_internal.h>

namespace stlv {

struct Clock {
    f64 start;
    f64 delta;
};

void clockClear(Clock& clock);
void clockStart(Clock& clock, f64 time);
void clockUpdate(Clock& clock, f64 time);

} // namespace stlv
