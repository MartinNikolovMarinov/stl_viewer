#pragma once


#include <iostream>

// #################### CORE LIBRARY CONFIGURATIONS ####################################################################

#define CORE_DEFAULT_ALLOCATOR() core::StdAllocator

#include <core.h>
#include <core.h> // If pragma once is omitted somewhere in the core library, this will catch it.

using namespace coretypes;

void coreInit();

template<> addr_size core::hash(const i32& key);
template<> bool core::eq(const i32& a, const i32& b);

template<> addr_size core::hash(const core::StrView& key);
template<> bool core::eq(const core::StrView& a, const core::StrView& b);

// ##################### Testing Helpers ###############################################################################

struct ConstructorTester {
    static constexpr i32 defaultValue = 7;

    i32 a;
    ConstructorTester() : a(defaultValue) { g_defaultCtorCalled++; }
    ConstructorTester(const ConstructorTester& other) : a(other.a) { g_copyCtorCalled++; }
    ConstructorTester(ConstructorTester&& other) : a(core::move(other.a)) { g_moveCtorCalled++; }
    ~ConstructorTester() { g_destructorsCalled++; }

    ConstructorTester& operator=(const ConstructorTester& other) {
        a = other.a;
        g_assignmentsCopyCalled++;
        return *this;
    }

    ConstructorTester& operator=(ConstructorTester&& other) {
        a = core::move(other.a);
        g_assignmentsMoveCalled++;
        return *this;
    }

    static void resetCtors() {
        g_defaultCtorCalled = 0;
        g_copyCtorCalled = 0;
        g_moveCtorCalled = 0;
    }

    static void resetDtors() {
        g_destructorsCalled = 0;
    }

    static void resetAssignments() {
        g_assignmentsCopyCalled = 0;
        g_assignmentsMoveCalled = 0;
    }

    static void resetAll() {
        resetCtors();
        resetDtors();
        resetAssignments();
    }

    static i32 defaultCtorCalled()      { return g_defaultCtorCalled; }
    static i32 copyCtorCalled()         { return g_copyCtorCalled; }
    static i32 moveCtorCalled()         { return g_moveCtorCalled; }
    static i32 totalCtorsCalled()       { return g_defaultCtorCalled + g_copyCtorCalled + g_moveCtorCalled; }
    static i32 assignmentsCopyCalled()  { return g_assignmentsCopyCalled; }
    static i32 assignmentsMoveCalled()  { return g_assignmentsMoveCalled; }
    static i32 assignmentsTotalCalled() { return g_assignmentsCopyCalled + g_assignmentsMoveCalled; }
    static i32 dtorsCalled()            { return g_destructorsCalled; }
    static bool noCtorsCalled()         { return totalCtorsCalled() == 0; }

private:
    inline static i32 g_destructorsCalled;
    inline static i32 g_defaultCtorCalled;
    inline static i32 g_copyCtorCalled;
    inline static i32 g_moveCtorCalled;
    inline static i32 g_assignmentsCopyCalled;
    inline static i32 g_assignmentsMoveCalled;
};

using CT = ConstructorTester;

struct StaticVariableDefaultCtorTester {
    // NOTE: This is a terrible idea, but it still should work.
    inline static i32 nextId = 0;
    i32 a = nextId++;
};

using SVCT = StaticVariableDefaultCtorTester;

/**
 * \brief This code is quite complex, because it does zero allocations, but it's purpose is quite simple. It iterates over
 *        a table of test cases, and executes the assertion function on each one. The assertion function, the test case
 *        table, and the error message prefix are all passed in as arguments. Every error message is pre-allocated on
 *        the stack, and the test case index is manipulated inside the char array.
 *
 * \param errMsgPrefix The error message prefix.
 * \param cases The test case table.
 * \param assertionFn The assertion function.
*/
template <addr_size PLen, typename TCase, addr_size NCases, typename Afunc>
constexpr void executeTestTable(const char (&errMsgPrefix)[PLen], const TCase (&cases)[NCases], Afunc assertionFn) {
    addr_size i = 0;
    char errMsg[PLen + 20] = {}; // The 20 is for the test case index number.
    for (addr_size j = 0; j < PLen; j++) { // NOTE: intentionally not using memcopy, because this needs to work in constexpr.
        errMsg[j] = errMsgPrefix[j];
    }
    char* appendIdx = &errMsg[PLen - 1];
    for (auto& c : cases) {
        core::intToCptr(i, appendIdx, 2);
        assertionFn(c, errMsg);
        i++;
    }
}

inline i32 g_testCount = 0;

#define RunTest(test, ...)                                                                      \
    g_testCount++;                                                                              \
    std::cout << "\t[TEST " << "№ " << g_testCount << " RUNNING] " << ANSI_BOLD(#test) << '\n'; \
    { [[maybe_unused]] auto __notused__ = test(__VA_ARGS__); }                                  \
    std::cout << "\t[TEST " << "№ " << g_testCount << ANSI_BOLD(ANSI_GREEN(" PASSED")) << "] " << ANSI_BOLD(#test) << std::endl;

#define RunTest_DisplayMemAllocs(test, allocator, ...)                                                                  \
    {                                                                                                                   \
        auto __a_before = allocator::totalAllocatedMem();                                                               \
        g_testCount++;                                                                                                  \
        std::cout << "\t[TEST " << "№ " << g_testCount << " RUNNING] " << ANSI_BOLD(#test) << '\n';                     \
        [[maybe_unused]] auto __notused__ = test(__VA_ARGS__);                                                          \
        std::cout << "\t[TEST " << "№ " << g_testCount << ANSI_BOLD(ANSI_GREEN(" PASSED")) << "] " << ANSI_BOLD(#test); \
        auto __a_after = allocator::totalAllocatedMem();                                                                \
        std::cout << " [TOTAL REQUESTED MEMORY]: " << (__a_after - __a_before) << " bytes" << std::endl;                \
    }

#if defined(CORE_RUN_COMPILETIME_TESTS) && CORE_RUN_COMPILETIME_TESTS == 1
    #define RunTestCompileTime(test, ...) \
        { [[maybe_unused]] constexpr auto __notused__ = core::force_consteval<test(__VA_ARGS__)>; }
#else
    #define RunTestCompileTime(...)
    #define CORE_RUN_COMPILETIME_TESTS 0
#endif

#define RunTestSuite(suite, ...)                                       \
    std::cout << "[SUITE RUNNING] " << ANSI_BOLD(#suite) << std::endl; \
    suite(__VA_ARGS__);                                                \
    std::cout << "[SUITE " << ANSI_BOLD(ANSI_GREEN("PASSED")) << "] " << ANSI_BOLD(#suite) << std::endl;

// ##################### Test suites ###################################################################################

i32 runAlgorithmsTestsSuite();
i32 runArrTestsSuite();
i32 runBitsTestsSuite();
i32 runCmdParserTestsSuite();
i32 runCptrConvTestsSuite();
i32 runCptrTestsSuite();
i32 runDeferTestsSuite();
i32 runExpectedTestsSuite();
i32 runHashMapTestsSuite();
i32 runHashTestsSuite();
i32 runIntrinsicsTestsSuite();
i32 runIntsTestsSuite();
i32 runMathTestsSuite();
i32 runMatrixTestsSuite();
i32 runMemTestsSuite();
i32 runRndTestsSuite();
i32 runStaticArrTestsSuite();
i32 runStrBuilderTestsSuite();
i32 runTraitsTestsSuite();
i32 runTransformsTestsSuite();
i32 runTupleTestsSuite();
i32 runUniquePtrTestsSuite();
i32 runUtfTestsSuite();
i32 runVecTestsSuite();

i32 runBumpAllocatorTestsSuite();
i32 runStdAllocatorTestsSuite();
i32 runStdStatsAllocatorTestsSuite();

i32 runPltErrorTestsSuite();
i32 runPltFileSystemTestsSuite();
i32 runPltPagesTestsSuite();
i32 runPltPathTestsSuite();
i32 runPltStacktraceTestsSuite();
i32 runPltThreadingTestsSuite();
i32 runPltTimeTestsSuite();

i32 runAllTests();
