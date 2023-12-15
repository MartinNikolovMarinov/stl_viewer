#include "../t-index.h"

#include <thread>
#include <iostream>

i32 getNumberOfCoresTest() {
    i32 n = ValueOrDie(core::threadingGetNumCores());
    Assert(n > 0);
    u32 stdN = std::thread::hardware_concurrency();
    Assert(u32(n) == stdN);
    return 0;
}

i32 threadSleepFor5msTest() {
    Expect(core::threadingSleep(5));
    return 0;
}

i32 threadNameingTest() {
    char buff[core::MAX_THREAD_NAME_LENGTH];

    {
        core::memset(buff, 0, core::MAX_THREAD_NAME_LENGTH);
        auto res = core::threadingGetName(buff);
        if (!res.hasErr()) {
            // Might have an error or might be empty.
            Assert(core::cptrEq(buff, "", core::cptrLen("")));
        }
    }

    {
        Expect(core::threadingSetName("First Name"));
        core::memset(buff, 0, core::MAX_THREAD_NAME_LENGTH);
        Expect(core::threadingGetName(buff));
        Assert(core::cptrEq(buff, "First Name", core::cptrLen("First Name")));
    }

    {
        Expect(core::threadingSetName("Name Change"));
        core::memset(buff, 0, core::MAX_THREAD_NAME_LENGTH);
        Expect(core::threadingGetName(buff));
        Assert(core::cptrEq(buff, "Name Change", core::cptrLen("Name Change")));
    }

    return 0;
}

i32 threadDetachDoesNotBreak() {
    core::Thread t;

    {
        auto res = core::threadDetach(t);
        Assert(res.hasErr(), "Detaching a thread that is not initialized should fail.");
        Assert(res.err() == core::ERR_THREAD_FAILED_TO_ACQUIRE_LOCK, "Invalid error code.");
    }

    Expect(core::threadInit(t));
    {
        auto res = core::threadDetach(t);
        Assert(res.hasErr(), "Detaching a thread that is not running should fail.");
        Assert(res.err() == core::ERR_THREAD_IS_NOT_JOINABLE_OR_DETACHABLE, "Invalid error code.");
    }

    Expect(core::threadStart(t, nullptr, [](void*) { core::threadingSleep(100); }));
    {
        auto res = core::threadDetach(t);
        Assert(!res.hasErr(), "Failed to detach thread.");
    }

    return 0;
}

i32 getCurrentThreadTest() {
    core::Thread t;
    Expect(core::threadingGetCurrent(t));
    Assert(core::threadIsRunning(t));
    return 0;
}

template <typename TAlloc>
i32 start2ThreadsAndJoinThemTest() {
    core::Thread t1;
    core::Thread t2;
    bool t1Done = false;
    bool t2Done = false;

    struct Args {
        bool* done;
    };

    {
        auto res = core::threadStart(t1, nullptr, [](void*) {});
        Assert(res.hasErr(), "Starting a thread that is not initialized should fail.");
        Assert(res.err() == core::ERR_THREAD_FAILED_TO_ACQUIRE_LOCK, "Invalid error code.");
    }

    Expect(core::threadInit(t1));
    Expect(core::threadInit(t2));

    Assert(core::threadIsRunning(t1) == false);
    Assert(core::threadIsRunning(t2) == false);

    // In this case arguments could be on the stack, because the threads are joined before the function returns, but
    // in general started threads will outlive the current stack frame, dus the lifetime of the arguments must be
    // managed on the heap.
    Args* t1Args = reinterpret_cast<Args*>(TAlloc::alloc(sizeof(Args)));
    Assert(t1Args != nullptr);
    *t1Args = {&t1Done};
    defer { TAlloc::free(t1Args); };
    Args* t2Args = reinterpret_cast<Args*>(TAlloc::alloc(sizeof(Args)));
    Assert(t2Args != nullptr);
    *t2Args = {&t2Done};
    defer { TAlloc::free(t2Args); };

    {
        auto res = core::threadStart<TAlloc>(t1, reinterpret_cast<void*>(t1Args), [](void* arg) {
            Args* args = reinterpret_cast<Args*>(arg);
            *args->done = true;
        });
        Assert(!res.hasErr());
    }

    {
        auto res = core::threadStart<TAlloc>(t2, reinterpret_cast<void*>(t2Args), [](void* arg) {
            Args* args = reinterpret_cast<Args*>(arg);
            *args->done = true;
        });
        Assert(!res.hasErr());
    }

    Expect(core::threadJoin(t1));
    Assert(core::threadIsRunning(t1) == false);
    Assert(t1Done == true);

    Expect(core::threadJoin(t2));
    Assert(core::threadIsRunning(t2) == false);
    Assert(t2Done == true);

    return 0;
}

template <typename TAlloc>
i32 mutexPreventsRaceConditionsTest() {
    core::Thread t1;
    core::Thread t2;
    i32 counter = 0;
    static core::Mutex mu;

    struct Args {
        i32* counter;
    };

    Expect(core::threadInit(t1));
    Expect(core::threadInit(t2));
    Expect(core::mutexInit(mu));

    Args* t1Args = reinterpret_cast<Args*>(TAlloc::alloc(sizeof(Args)));
    *t1Args = Args{&counter};
    defer { TAlloc::free(t1Args); };
    Args* t2Args = reinterpret_cast<Args*>(TAlloc::alloc(sizeof(Args)));
    *t2Args = Args{&counter};
    defer { TAlloc::free(t2Args); };

    {
        auto res = core::threadStart<TAlloc>(t1, reinterpret_cast<void*>(t1Args), [](void* arg) {
            Args* args = reinterpret_cast<Args*>(arg);
            for (i32 i = 0; i < 100000; ++i) {
                Expect(core::mutexLock(mu));

                i32& c = *args->counter;
                ++c;

                Expect(core::mutexUnlock(mu));
            }
        });
        Assert(!res.hasErr());
    }

    {
        auto res = core::threadStart<TAlloc>(t2, reinterpret_cast<void*>(t2Args), [](void* arg) {
            Args* args = reinterpret_cast<Args*>(arg);
            for (i32 i = 0; i < 100000; ++i) {
                Expect(core::mutexLock(mu));

                i32& c = *args->counter;
                ++c;

                Expect(core::mutexUnlock(mu));
            }
        });
        Assert(!res.hasErr());
    }

    Expect(core::threadJoin(t1));
    Expect(core::threadJoin(t2));

    Assert(counter == 200000, "Race condition was not prevented by the mutex.");

    return 0;
}

template <typename TAlloc>
i32 arrayOfMutexesAndThreadsTest() {
    constexpr addr_size N = 4;
    core::Arr<core::Mutex, TAlloc> mutexes (N - 1);
    core::Arr<core::Thread, TAlloc> threads (N - 1);

    mutexes.append(core::Mutex{});
    threads.append(core::Thread{});

    // Init all objects.
    for (addr_size i = 0; i < N; ++i) {
        Expect(core::mutexInit(mutexes[i]));
        Expect(core::threadInit(threads[i]));
    }

    // Start all threads.
    for (addr_size i = 0; i < N; ++i) {
        // Check that the threads can be started.
        Expect(core::threadStart<TAlloc>(threads[i], nullptr, [](void*) { core::threadingSleep(5); }));

        // Check that the mutexes can be locked and unlocked.
        Expect(core::mutexLock(mutexes[i]));
        Expect(core::mutexUnlock(mutexes[i]));
    }

    // Join all threads.
    for (addr_size i = 0; i < N; ++i) {
        auto& curr = threads[i];
        Expect(core::threadJoin(curr));
    }

    return 0;
}

template <typename TAlloc>
i32 condVarSignalThreadToContinueWorkTest() {
    core::Thread t;
    core::CondVariable cv;

    Expect(core::threadInit(t));
    Expect(core::condVarInit(cv));
    defer { Expect(core::condVarDestroy(cv)); };

    core::threadStart<TAlloc>(t, &cv, [](void* arg) {
        core::CondVariable* cvar = reinterpret_cast<core::CondVariable*>(arg);
        core::Mutex mu;

        Expect(core::mutexInit(mu));
        defer { Expect(core::mutexDestroy(mu)); };

        Expect(core::mutexLock(mu));
        defer { Expect(core::mutexUnlock(mu)); };

        // Wait for signal
        Expect(core::condVarWaitTimed(*cvar, mu, 1000)); // 1 second
    });

    Expect(core::threadingSleep(50));

    Expect(core::condVarSignal(cv));

    Expect(core::threadJoin(t));

    return 0;
}

i32 runPltThreadingTestsSuite() {
    constexpr addr_size BUFF_SIZE = core::KILOBYTE;
    char buf[BUFF_SIZE];

    core::StdAllocator::init(nullptr);
    core::StdStatsAllocator::init(nullptr);
    core::BumpAllocator::init(nullptr, buf, BUFF_SIZE);

    RunTest(getNumberOfCoresTest);
    RunTest(threadSleepFor5msTest);
    RunTest(threadNameingTest);
    RunTest(threadDetachDoesNotBreak);
    RunTest(getCurrentThreadTest);

    // NOTE: The custom allocators are not thread safe, so the only possible test is with the std allocator.

    RunTest_DisplayMemAllocs(start2ThreadsAndJoinThemTest<core::StdAllocator>, core::StdAllocator);
    RunTest_DisplayMemAllocs(mutexPreventsRaceConditionsTest<core::StdAllocator>, core::StdAllocator);
    RunTest_DisplayMemAllocs(arrayOfMutexesAndThreadsTest<core::StdAllocator>, core::StdAllocator);
    RunTest_DisplayMemAllocs(condVarSignalThreadToContinueWorkTest<core::StdAllocator>, core::StdAllocator);

    return 0;
}
