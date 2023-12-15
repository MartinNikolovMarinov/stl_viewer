#include "../t-index.h"

template <typename TAllocator>
i32 callingStacktraceDoesNotCrashTest() {
    constexpr addr_size BUF_MAX = 4096;
    char buf[BUF_MAX] = {};
    addr_size bufWritten = 0;
    bool ok = core::stacktrace<TAllocator>(buf, BUF_MAX, bufWritten, 20);

    Assert(ok);
    Assert(bufWritten > 0);
    Assert(bufWritten < BUF_MAX);
    Assert(buf[0] != '\0');
    Assert(core::cptrLen(buf) == bufWritten);

    return 0;
}

template <typename TAllocator>
i32 stacktraceContainsThisFunctionTest() {
    constexpr addr_size BUF_MAX = 4096;
    char buf[BUF_MAX] = {};
    addr_size bufWritten = 0;
    bool ok = core::stacktrace<TAllocator>(buf, BUF_MAX, bufWritten, 3);

    Assert(ok);
    Assert(bufWritten > 0);
    Assert(bufWritten < BUF_MAX);
    Assert(buf[0] != '\0');
    Assert(core::cptrLen(buf) == bufWritten);

    std::string traceStr(buf, bufWritten);
    Assert(traceStr.find("stacktraceContainsThisFunctionTest") != std::string::npos);
    Assert(traceStr.find("runPltStacktraceTestsSuite") != std::string::npos);

    return 0;
}

template <typename TAllocator>
inline i32 stacktraceOnInlinedTest() {
    constexpr addr_size BUF_MAX = 4096;
    char buf[BUF_MAX] = {};
    addr_size bufWritten = 0;
    bool ok = core::stacktrace<TAllocator>(buf, BUF_MAX, bufWritten, 3);

    Assert(ok);
    Assert(bufWritten > 0);
    Assert(bufWritten < BUF_MAX);
    Assert(buf[0] != '\0');
    Assert(core::cptrLen(buf) == bufWritten);

    std::string traceStr(buf, bufWritten);
    Assert(traceStr.find("runPltStacktraceTestsSuite") != std::string::npos);

    return 0;
}

i32 runPltStacktraceTestsSuite() {
    constexpr addr_size BUFF_SIZE = core::KILOBYTE;
    char buf[BUFF_SIZE];

    core::StdAllocator::init(nullptr);
    core::StdStatsAllocator::init(nullptr);
    core::BumpAllocator::init(nullptr, buf, BUFF_SIZE);

    auto checkLeaks = []() {
        Assert(core::StdAllocator::usedMem() == 0);
        Assert(core::StdStatsAllocator::usedMem() == 0, "Memory leak detected!");
        Assert(core::BumpAllocator::usedMem() == 0);
    };

    {
        RunTest_DisplayMemAllocs(callingStacktraceDoesNotCrashTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(callingStacktraceDoesNotCrashTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(callingStacktraceDoesNotCrashTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(stacktraceContainsThisFunctionTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(stacktraceContainsThisFunctionTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(stacktraceContainsThisFunctionTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(stacktraceOnInlinedTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(stacktraceOnInlinedTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(stacktraceOnInlinedTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }

    return 0;
}
