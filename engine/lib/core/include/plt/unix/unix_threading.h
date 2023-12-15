#pragma once

#include <plt/core_threading.h>

#include <pthread.h>

namespace core {

struct Mutex {
    pthread_mutex_t handle;
};

struct CondVariable {
    pthread_cond_t handle;
};

struct Thread {
    pthread_t handle;
    bool isRunning;
    mutable Mutex mu;
    AtomicBool canLock;

    NO_COPY(Thread);

    Thread() noexcept : handle(pthread_t()), isRunning(false) {}

    // only move
    Thread(Thread&& other) noexcept : handle(other.handle) {
        other.handle = 0;
    }

    Thread& operator=(Thread&& other) noexcept {
        handle = other.handle;
        other.handle = 0;
        return *this;
    }
};

namespace detail {

struct ThreadInfo {
    ThreadRoutine routine;
    void* arg;
    void (*destroy)(void*);
};

inline void* proxy(void* arg) {

    ThreadInfo* tinfo = reinterpret_cast<ThreadInfo*>(arg);
    tinfo->routine(tinfo->arg);
    tinfo->destroy(tinfo);

    // NOTE: If the routine crashes or throws an unhandled exception the thread info will be leaked. It is better to
    //       prevent such cases in the routine itself.

    return nullptr;
}

} // namespace detail

template <typename TAlloc>
expected<PltErrCode> threadStart(Thread& out, void* arg, ThreadRoutine routine) noexcept {
    using namespace detail;

    if (!out.canLock.load(std::memory_order_acquire)) {
        return core::unexpected(ERR_THREAD_FAILED_TO_ACQUIRE_LOCK);
    }

    if (threadIsRunning(out)) {
        return core::unexpected(ERR_THREADING_STARTING_AN_ALREADY_RUNNING_THREAD);
    }

    Expect(mutexLock(out.mu));
    defer { Expect(mutexUnlock(out.mu)); };

    ThreadInfo* tinfo = reinterpret_cast<ThreadInfo*>(TAlloc::alloc(sizeof(ThreadInfo)));
    tinfo->routine = routine;
    tinfo->arg = arg;
    tinfo->destroy = [] (void* ptr) {
        ThreadInfo* info = reinterpret_cast<ThreadInfo*>(ptr);
        TAlloc::free(info);
    };
    if (tinfo == nullptr) {
        return core::unexpected(ERR_ALLOCATOR_DEFAULT_NO_MEMORY);
    }

    i32 res = pthread_create(&out.handle, nullptr, proxy, reinterpret_cast<void*>(tinfo));
    if (res != 0) {
        out.handle = pthread_t();
        TAlloc::free(tinfo);
        return core::unexpected(PltErrCode(res));
    }

    out.isRunning = true;
    return {};
}

}; // namespace core
