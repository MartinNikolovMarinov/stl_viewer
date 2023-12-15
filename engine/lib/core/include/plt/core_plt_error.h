#pragma once

#include <core_API.h>
#include <core_types.h>

namespace core {

using namespace coretypes;

using PltErrCode = i64;

namespace detail {

// Custom library errors start from this value.
constexpr PltErrCode ERR_START_OF_CUSTOM_ERRORS = PltErrCode(0xffffffff00000000LL);

constexpr PltErrCode ERR_MISC_CUSTOM_ERROR_START = detail::ERR_START_OF_CUSTOM_ERRORS + 0;
constexpr PltErrCode ERR_OS_CUSTOM_ERROR_START = detail::ERR_START_OF_CUSTOM_ERRORS + 100;
constexpr PltErrCode ERR_THREAD_CUSTOM_ERROR_START = detail::ERR_START_OF_CUSTOM_ERRORS + 200;

} // namespace detail

constexpr PltErrCode ERR_PLT_NONE = 0;

constexpr PltErrCode ERR_ALLOCATOR_DEFAULT_NO_MEMORY = detail::ERR_MISC_CUSTOM_ERROR_START + 0;

constexpr PltErrCode ERR_THREADING_INVALID_THREAD_NAME = detail::ERR_THREAD_CUSTOM_ERROR_START + 0;
constexpr PltErrCode ERR_THREADING_STARTING_AN_ALREADY_RUNNING_THREAD = detail::ERR_THREAD_CUSTOM_ERROR_START + 1;
constexpr PltErrCode ERR_THREAD_IS_NOT_JOINABLE_OR_DETACHABLE = detail::ERR_THREAD_CUSTOM_ERROR_START + 2;
constexpr PltErrCode ERR_THREAD_FAILED_TO_ACQUIRE_LOCK = detail::ERR_THREAD_CUSTOM_ERROR_START + 3;
constexpr PltErrCode ERR_MUTEX_TRYLOCK_FAILED = detail::ERR_THREAD_CUSTOM_ERROR_START + 4;

constexpr char const* customPltErrorDescribe(PltErrCode err) {
    if (err < detail::ERR_START_OF_CUSTOM_ERRORS) {
        return "Not a custom user error";
    }

    switch (err) {
        case ERR_ALLOCATOR_DEFAULT_NO_MEMORY:                  return "Default allocator ran out of memory";

        case ERR_THREADING_INVALID_THREAD_NAME:                return "Invalid thread name";
        case ERR_THREADING_STARTING_AN_ALREADY_RUNNING_THREAD: return "Starting an already running thread";
        case ERR_THREAD_IS_NOT_JOINABLE_OR_DETACHABLE:         return "Thread is not joinable or detachable";
        case ERR_THREAD_FAILED_TO_ACQUIRE_LOCK:                return "Thread failed to acquire lock.";
        case ERR_MUTEX_TRYLOCK_FAILED:                         return "Mutex trylock failed";
    }

    return "Unknown error";
}

// Error descriptions provided by the OS.
constexpr addr_size MAX_SYSTEM_ERR_MSG_SIZE = 512;
CORE_API_EXPORT bool pltErrorDescribe(PltErrCode err, char out[MAX_SYSTEM_ERR_MSG_SIZE]);

} // namespace core
