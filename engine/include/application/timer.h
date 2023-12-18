#pragma once

#include <fwd_internal.h>

namespace stlv {

struct Timer {
    f64 start;
    f64 delta;
};

STLV_EXPORT void clockClear(Timer& clock);
STLV_EXPORT void clockStart(Timer& clock, f64 time);
STLV_EXPORT void clockUpdate(Timer& clock, f64 time);

} // namespace stlv
