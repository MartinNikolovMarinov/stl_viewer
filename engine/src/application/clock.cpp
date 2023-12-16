#include <application/clock.h>

namespace stlv {

void clockStart(Clock& clock, f64 time) {
    clock.start = time;
    clock.delta = 0.0;
}

void clockUpdate(Clock& clock, f64 time) {
    if (clock.start > 0.0) {
        clock.delta = time - clock.start;
    }
}

void clockClear(Clock& clock) {
    clock.start = -1;
    clock.delta = 0.0;
}

} // namespace stlv
