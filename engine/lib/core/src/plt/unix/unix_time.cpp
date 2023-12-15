#include <plt/core_time.h>

#include <errno.h>
#include <time.h>

namespace core {

expected<u64, PltErrCode> getCurrentUnixTimestampMs() {
    timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return core::unexpected(PltErrCode(errno));
    }

    u64 res = u64(ts.tv_sec * 1000) + u64(ts.tv_nsec) / 1000000;
    return res;
}

} // namespace core
