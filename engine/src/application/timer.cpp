#include <application/timer.h>

namespace stlv {

void clockStart(Timer& clock, f64 time) {
    clock.start = time;
    clock.delta = 0.0;
}

void clockUpdate(Timer& clock, f64 time) {
    if (clock.start > 0.0) {
        clock.delta = time - clock.start;
    }
}

void clockClear(Timer& clock) {
    clock.start = -1;
    clock.delta = 0.0;
}

} // namespace stlv
