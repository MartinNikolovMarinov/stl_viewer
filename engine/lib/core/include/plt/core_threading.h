#pragma once

#include <core_alloc.h>
#include <core_API.h>
#include <core_expected.h>
#include <core_types.h>
#include <plt/core_plt_error.h>

#include <atomic>

namespace core {

using namespace coretypes;

using AtomicChar = std::atomic<char>;
using AtomicI32  = std::atomic<i32>;
using AtomicI64  = std::atomic<i64>;
using AtomicU32  = std::atomic<u32>;
using AtomicU64  = std::atomic<u64>;
using AtomicBool = std::atomic<bool>;
using AtomicPtr  = std::atomic<void*>;

struct Thread;

using ThreadRoutine = void (*)(void*);

// NOTE: The Linux kernel limits thread names to 16 characters (including the null terminator).
constexpr u32 MAX_THREAD_NAME_LENGTH = 16;

CORE_API_EXPORT expected<i32, PltErrCode> threadingGetNumCores() noexcept;
CORE_API_EXPORT expected<PltErrCode>      threadingGetCurrent(Thread& out) noexcept;
CORE_API_EXPORT expected<PltErrCode>      threadingSleep(u64 ms) noexcept;
CORE_API_EXPORT expected<PltErrCode>      threadingSetName(const char* name) noexcept;
CORE_API_EXPORT expected<PltErrCode>      threadingGetName(char out[MAX_THREAD_NAME_LENGTH]) noexcept;
CORE_API_EXPORT void                      threadingExit(i32 exitCode) noexcept;

/**
 * @brief Starts a thread.
 *
 * @note Must be Thread Safe.
 *
 * @param out The previously initialized thread object.
 * @param arg The argument to pass to the thread routine. The lifetime of this argument is managed by the user and must
 *            be longer than the thread.
 * @param routine The routine to run in the thread.
 *
 * @return An error code if the thread could not be started.
*/
template <typename TAlloc = CORE_DEFAULT_ALLOCATOR()>
expected<PltErrCode> threadStart(Thread& out, void* arg, ThreadRoutine routine) noexcept;

/**
 * @brief Initializes a thread object.
 *
 * @param out The thread object to initialize.
 *
 * @return An error code if the thread could not be initialized.
*/
CORE_API_EXPORT expected<PltErrCode> threadInit(Thread& t) noexcept;

/**
 * @brief Checks if a thread is running.
 *
 * @note Must be Thread Safe.
 *
 * @param t The thread to check.
 *
 * @return True if the thread is running, false otherwise.
*/
CORE_API_EXPORT bool threadIsRunning(const Thread& t) noexcept;

/**
 * @brief Checks if two threads are equal.
 *
 * @param t1 The first thread.
 * @param t2 The second thread.
 *
 * @return True if the threads are equal, false otherwise and an error code if the threads could not be compared.
*/
CORE_API_EXPORT expected<bool, PltErrCode> threadEq(const Thread& t1, const Thread& t2) noexcept;

/**
 * @brief Joins a thread.
 *
 * @note Must be Thread Safe.
 *
 * @param t The thread to join.
 *
 * @return An error code if the thread could not be joined.
*/
CORE_API_EXPORT expected<PltErrCode> threadJoin(Thread& t) noexcept;

/**
 * @brief Detaches a thread.
 *
 * @note Must be Thread Safe.
 *
 * @param t The thread to detach.
 *
 * @return An error code if the thread could not be detached.
*/
CORE_API_EXPORT expected<PltErrCode> threadDetach(Thread& t) noexcept;

struct Mutex;

enum struct MutexType : u8 {
    Normal,
    Recursive,
    ErrorCheck
};

CORE_API_EXPORT expected<PltErrCode> mutexInit(Mutex& out, MutexType type = MutexType::Normal) noexcept;
CORE_API_EXPORT expected<PltErrCode> mutexDestroy(Mutex& m) noexcept;
CORE_API_EXPORT expected<PltErrCode> mutexLock(Mutex& m) noexcept;
CORE_API_EXPORT expected<PltErrCode> mutexTrylock(Mutex& m) noexcept;
CORE_API_EXPORT expected<PltErrCode> mutexUnlock(Mutex& m) noexcept;

struct CondVariable;

CORE_API_EXPORT expected<PltErrCode> condVarInit(CondVariable& out) noexcept;
CORE_API_EXPORT expected<PltErrCode> condVarDestroy(CondVariable& cv) noexcept;
CORE_API_EXPORT expected<PltErrCode> condVarWaitTimed(CondVariable& cv, Mutex& m, u64 ms) noexcept;
CORE_API_EXPORT expected<PltErrCode> condVarSignal(CondVariable& cv) noexcept;
CORE_API_EXPORT expected<PltErrCode> condVarBroadcast(CondVariable& cv) noexcept;

} // namespace core

#if defined(OS_WIN) && OS_WIN == 1
    #include <plt/win/win_threading.h>
#elif defined(OS_LINUX) && OS_LINUX == 1
    #include <plt/unix/unix_threading.h>
#elif defined(OS_MAC) && OS_MAC == 1
    #include <plt/unix/unix_threading.h>
#endif
