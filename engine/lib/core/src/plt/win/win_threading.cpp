#include <plt/core_threading.h>
#include <plt/win/win_threading.h>

#include <core_cptr.h>
#include <core_alloc.h>

#include <windows.h>

namespace core {

// Mutex Implementation

expected<PltErrCode> mutexInit(Mutex& out, MutexType) noexcept {
    // There is only one type of mutex in Windows.
    InitializeCriticalSection(&out.handle);
    return {};
}

expected<PltErrCode> mutexDestroy(Mutex& m) noexcept {
    DeleteCriticalSection(&m.handle);
    return {};
}

expected<PltErrCode> mutexLock(Mutex& m) noexcept {
    EnterCriticalSection(&m.handle);
    return {};
}

expected<PltErrCode> mutexTrylock(Mutex& m) noexcept {
    BOOL ret = TryEnterCriticalSection(&m.handle);
    if (!ret) {
        return core::unexpected(ERR_MUTEX_TRYLOCK_FAILED);
    }
    return {};
}

expected<PltErrCode> mutexUnlock(Mutex& m) noexcept {
    LeaveCriticalSection(&m.handle);
    return {};
}

// Thread Implementation

expected<i32, PltErrCode> threadingGetNumCores() noexcept {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    i32 n = i32(sysInfo.dwNumberOfProcessors);
    if (n <= 0) {
        // sysconf sets errno on errors.
        return core::unexpected(PltErrCode(GetLastError()));
    }

    return n;
}

expected<PltErrCode> threadingGetCurrent(Thread& out) noexcept {
    // For Windows, the GetCurrentThread() returns a pseudo handle to the current thread and not an actual handle.
    // To convert this pseudo handle into a real handle, DuplicateHandle() is used.

    if (out.isRunning) {
        return core::unexpected(ERR_THREADING_STARTING_AN_ALREADY_RUNNING_THREAD);
    }

    if (auto res = core::threadInit(out); res.hasErr()) {
        return core::unexpected(res.err());
    }

    HANDLE pseudoHandle = GetCurrentThread();
    bool ret = DuplicateHandle(GetCurrentProcess(), pseudoHandle, GetCurrentProcess(), &out.handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    if (!ret) {
        out.handle = nullptr;
        return core::unexpected(PltErrCode(GetLastError()));
    }
    return {};
}

expected<PltErrCode> threadingSleep(u64 ms) noexcept {
    Panic(ms <= u64(core::MAX_I32) && "Sleep time is too large");
    Sleep(DWORD(ms));
    return {};
}

expected<PltErrCode> threadingSetName(const char* name) noexcept {
    if (name == nullptr) {
        return core::unexpected(ERR_THREADING_INVALID_THREAD_NAME);
    }
    addr_size len = core::cptrLen(name);
    if (len > MAX_THREAD_NAME_LENGTH) {
        return core::unexpected(ERR_THREADING_INVALID_THREAD_NAME);
    }

    wchar_t wname[MAX_THREAD_NAME_LENGTH + 1];
    int ret = MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, MAX_THREAD_NAME_LENGTH + 1);
    if (ret <= 0) { // Check for error in conversion
        return core::unexpected(PltErrCode(GetLastError()));
    }
    Assert(ret <= MAX_THREAD_NAME_LENGTH + 1, "MultiByteToWideChar() returned a value greater than the buffer size");

    HRESULT hr = SetThreadDescription(GetCurrentThread(), wname);
    if (FAILED(hr)) {
        return core::unexpected(PltErrCode(hr)); // Directly use HRESULT for error code
    }

    return {};
}

expected<PltErrCode> threadingGetName(char out[MAX_THREAD_NAME_LENGTH]) noexcept {
    PWSTR wname = nullptr;
    HRESULT hr = GetThreadDescription(GetCurrentThread(), &wname);
    if (FAILED(hr)) {
        return core::unexpected(PltErrCode(hr));
    }

    int ret = WideCharToMultiByte(CP_UTF8, 0, wname, -1, out, MAX_THREAD_NAME_LENGTH, nullptr, nullptr);

    LocalFree(wname); // Free the memory allocated by GetThreadDescription !

    if (ret <= 0) {
        return core::unexpected(PltErrCode(GetLastError()));
    }
    Assert(ret <= MAX_THREAD_NAME_LENGTH, "WideCharToMultiByte() returned a value greater than the buffer size");

    return {};
}

void threadingExit(i32 code) noexcept {
    ExitThread(DWORD(code));
}

expected<PltErrCode> threadInit(Thread& t) noexcept {
    if (t.canLock.load(std::memory_order_acquire)) {
        return {};
    }
    auto ret = mutexInit(t.mu);
    if (ret.hasErr()) {
        return core::unexpected(ret.err());
    }
    t.canLock.store(true);
    return {};
}

bool threadIsRunning(const Thread& t) noexcept {
    if (!t.canLock.load(std::memory_order_acquire)) {
        return false;
    }

    Expect(mutexLock(t.mu));
    defer { Expect(mutexUnlock(t.mu)); };

    if (t.handle == nullptr) {
        return false;
    }
    DWORD exitCode = 0;
    if (GetExitCodeThread(t.handle, &exitCode)) {
        return (exitCode == STILL_ACTIVE);
    }

    return false; // If GetExitCodeThread fails, assume the thread is not running.
}

expected<bool, PltErrCode> threadEq(const Thread& t1, const Thread& t2) noexcept {
    bool ret = (t1.handle == t2.handle);
    return ret;
}

expected<PltErrCode> threadJoin(Thread& t) noexcept {
    if (!t.canLock.load(std::memory_order_acquire)) {
        return core::unexpected(ERR_THREAD_FAILED_TO_ACQUIRE_LOCK);
    }

    Expect(mutexLock(t.mu));

    if (!t.isRunning) {
        Expect(mutexUnlock(t.mu));
        return core::unexpected(ERR_THREAD_IS_NOT_JOINABLE_OR_DETACHABLE);
    }

    DWORD res = WaitForSingleObject(t.handle, INFINITE);
    if (res == WAIT_FAILED) {
        Expect(mutexUnlock(t.mu));
        return core::unexpected(PltErrCode(GetLastError()));
    }

    CloseHandle(t.handle);
    t.handle = nullptr;
    t.isRunning = false;

    t.canLock.store(false);

    Expect(mutexUnlock(t.mu));
    Expect(mutexDestroy(t.mu));

    return {};
}

expected<PltErrCode> threadDetach(Thread& t) noexcept {
    if (!t.canLock.load(std::memory_order_acquire)) {
        return core::unexpected(ERR_THREAD_FAILED_TO_ACQUIRE_LOCK);
    }

    Expect(mutexLock(t.mu));

    if (!t.isRunning) {
        Expect(mutexUnlock(t.mu));
        return core::unexpected(ERR_THREAD_IS_NOT_JOINABLE_OR_DETACHABLE);
    }

    if (!CloseHandle(t.handle)) {
        Expect(mutexUnlock(t.mu));
        return core::unexpected(PltErrCode(GetLastError()));
    }

    t.handle = nullptr;
    t.isRunning = false;

    t.canLock.store(false);

    Expect(mutexUnlock(t.mu));
    Expect(mutexDestroy(t.mu));

    return {};
}

// Condition Variable Implementation

expected<PltErrCode> condVarInit(CondVariable& out) noexcept {
    InitializeConditionVariable(&out.handle);
    return {};
}

expected<PltErrCode> condVarDestroy(CondVariable&) noexcept {
    // Windows does not require explicit destruction of CONDITION_VARIABLE
    // The function is kept for API consistency
    return {};
}

expected<PltErrCode> condVarWaitTimed(CondVariable& cv, Mutex& m, u64 ms) noexcept {
    BOOL res = SleepConditionVariableCS(&cv.handle, &m.handle, static_cast<DWORD>(ms));
    if (!res) {
        return core::unexpected(PltErrCode(GetLastError()));
    }
    return {};
}

expected<PltErrCode> condVarSignal(CondVariable& cv) noexcept {
    WakeConditionVariable(&cv.handle);
    return {};
}

expected<PltErrCode> condVarBroadcast(CondVariable& cv) noexcept {
    WakeAllConditionVariable(&cv.handle);
    return {};
}

} // namespace core
