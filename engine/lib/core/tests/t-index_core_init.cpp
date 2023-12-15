#include "t-index.h"

void coreInit() {
    core::setGlobalAssertHandler([](const char* failedExpr, const char* file, i32 line, const char* errMsg) {
        constexpr u32 stackFramesToSkip = 3;
        constexpr addr_size stackTraceBufferSize = 4096;
        char trace[stackTraceBufferSize] = {};
        addr_size traceLen = 0;
        core::stacktrace(trace, stackTraceBufferSize, traceLen, 200, stackFramesToSkip);
        std::cout << ANSI_RED_START() << ANSI_BOLD_START()
                  << "[ASSERTION]:\n  [EXPR]: " << failedExpr
                  << "\n  [FILE]: " << file << ":" << line
                  << "\n  [MSG]: " << errMsg
                  << ANSI_RESET()
                  << std::endl;
        std::cout << ANSI_BOLD_START() << "[TRACE]:\n" << trace << ANSI_RESET() << std::endl;
        throw std::runtime_error("Assertion failed!");
    });
}

template<>
addr_size core::hash(const i32& key) {
    addr_size h = addr_size(core::simpleHash_32(reinterpret_cast<const void*>(&key), sizeof(key)));
    return h;
}

template<>
bool core::eq(const i32& a, const i32& b) {
    return a == b;
}

template<>
addr_size core::hash(const core::StrView& key) {
    addr_size h = addr_size(core::simpleHash_32(key.data(), key.len()));
    return h;
}

template<>
bool core::eq(const core::StrView& a, const core::StrView& b) {
    return a.eq(b);
}

i32 runAllTests() {
    coreInit();

    std::cout << "\n" << "RUNNING TESTS" << "\n\n";

    RunTestSuite(runAlgorithmsTestsSuite);
    RunTestSuite(runArrTestsSuite);
    RunTestSuite(runBitsTestsSuite);
    RunTestSuite(runCmdParserTestsSuite);
    RunTestSuite(runCptrConvTestsSuite);
    RunTestSuite(runCptrTestsSuite);
    RunTestSuite(runDeferTestsSuite);
    RunTestSuite(runExpectedTestsSuite);
    RunTestSuite(runHashMapTestsSuite);
    RunTestSuite(runHashTestsSuite);
    RunTestSuite(runIntrinsicsTestsSuite);
    RunTestSuite(runIntsTestsSuite);
    RunTestSuite(runMathTestsSuite);
    RunTestSuite(runMatrixTestsSuite);
    RunTestSuite(runMemTestsSuite);
    RunTestSuite(runRndTestsSuite);
    RunTestSuite(runStaticArrTestsSuite);
    RunTestSuite(runStrBuilderTestsSuite);
    RunTestSuite(runTraitsTestsSuite);
    RunTestSuite(runTransformsTestsSuite);
    RunTestSuite(runTupleTestsSuite);
    RunTestSuite(runUniquePtrTestsSuite);
    RunTestSuite(runUtfTestsSuite);
    RunTestSuite(runVecTestsSuite);

    RunTestSuite(runBumpAllocatorTestsSuite);
    RunTestSuite(runStdAllocatorTestsSuite);
    RunTestSuite(runStdStatsAllocatorTestsSuite);

    // Run platform specific tests:

    #if defined(CORE_DEBUG) && CORE_DEBUG == 1
        // Stacktrace should only be expected to work in debug builds.
        RunTestSuite(runPltStacktraceTestsSuite);
    #endif
    RunTestSuite(runPltThreadingTestsSuite);
    RunTestSuite(runPltTimeTestsSuite);
    RunTestSuite(runPltErrorTestsSuite);
    RunTestSuite(runPltPagesTestsSuite);
    RunTestSuite(runPltFileSystemTestsSuite);

    std::cout << '\n';
    std::cout << ANSI_BOLD(ANSI_GREEN("Tests OK")) << std::endl;

    if constexpr (CORE_RUN_COMPILETIME_TESTS != 1) {
        std::cout << ANSI_YELLOW_START() << ANSI_BOLD_START()
                  << "[WARN] DID NOT RUN COMPILETIME TESTS!"
                  << ANSI_RESET() << std::endl;
    }

    return 0;
}
