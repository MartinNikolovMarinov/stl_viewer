#include <plt/core_time.h>

#include <windows.h>

namespace core {

expected<u64, PltErrCode> getCurrentUnixTimestampMs() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    u64 timeNowIn100thNanoseconds;
    timeNowIn100thNanoseconds = u64(ft.dwLowDateTime);
    timeNowIn100thNanoseconds += u64(ft.dwHighDateTime) << 32;

    // January 1, 1601 (UTC) is the epoch for FILETIME.
    // January 1, 1970 (UTC) is the epoch for Unix time.
    // The FILETIME epoch needs to be converted to Unix epoch.

    // This magic number is the number of 100 nanosecond intervals between the two epochs:
    const u64 START_OF_EPOCH_IN_NS = 116444736000000000ULL;

    // Subtract the epoch to get the Unix timestamp since 1970:
    timeNowIn100thNanoseconds -= START_OF_EPOCH_IN_NS;

    // Convert to milliseconds:
    u64 timeNowMs = timeNowIn100thNanoseconds / 10000;

    return timeNowMs;
}

}
