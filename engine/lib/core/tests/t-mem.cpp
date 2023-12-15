#include "t-index.h"

constexpr i32 alignTest() {
    struct TestCase {
        addr_size in;
        addr_size expected;
    };

    constexpr TestCase cases[] = {
        { 0, 0 },
        { 1, 8 },
        { 2, 8 },
        { 3, 8 },
        { 4, 8 },
        { 5, 8 },
        { 6, 8 },
        { 7, 8 },
        { 8, 8 },
        { 9, 16 },
        { 10, 16 },
        { 17, 24 },
        { 25, 32 }
    };

    executeTestTable("alignTest failed at index: ", cases, [](auto& c, const char* cErr) {
        auto got = core::align(c.in);
        Assert(got == c.expected, cErr);
    });

    return 0;
}

PRAGMA_WARNING_PUSH

DISABLE_GCC_AND_CLANG_WARNING(-Wconversion)
DISABLE_MSVC_WARNING(4244)

i32 swapBytesTest() {
    auto runTestCase = [](auto& a, auto& b, addr_size N) {
        for (addr_size i = 0; i < N; ++i) {
            a[i] = i;
            b[i] = i + N;
        }

        core::swapBytes(a, b, N);

        for (addr_size i = 0; i < N; ++i) {
            Assert(a[i] == i + N);
            Assert(b[i] == i);
        }
    };

    {
        // small
        u8 a[1] = {};
        u8 b[1] = {};
        runTestCase(a, b, 1);
    }
    {
        // odd
        u8 a[5] = {};
        u8 b[5] = {};
        runTestCase(a, b, 5);
    }
    {
        // even
        u8 a[6] = {};
        u8 b[6] = {};
        runTestCase(a, b, 6);
    }
    {
        struct A {
            i32 a;
            u64 b;
            u8 c;
        };

        A a1 = { core::MAX_I32, core::MAX_U64, core::MAX_U8 };
        A a2 = { core::MIN_I32, 0, 0 };
        core::swapBytes(&a1, &a2, sizeof(A));
        Assert(a1.a == core::MIN_I32);
        Assert(a1.b == 0);
        Assert(a1.c == 0);
        Assert(a2.a == core::MAX_I32);
        Assert(a2.b == core::MAX_U64);
        Assert(a2.c == core::MAX_U8);
    }
    {
        struct A {
            i32 a;
            u8 b[3];
            i32 c;
        };

        A a1 = { 1, {2, 3, 4}, 5 };
        A a2 = { 5, {6, 7, 8}, 9 };
        core::swapBytes(&a1, &a2, sizeof(A));
        Assert(a1.a == 5);
        Assert(a1.b[0] == 6);
        Assert(a1.b[1] == 7);
        Assert(a1.b[2] == 8);
        Assert(a1.c == 9);
        Assert(a2.a == 1);
        Assert(a2.b[0] == 2);
        Assert(a2.b[1] == 3);
        Assert(a2.b[2] == 4);
        Assert(a2.c == 5);
    }

    return 0;
}

PRAGMA_WARNING_POP

i32 memcopyTest() {
    constexpr i32 N = 20;
    u8 sequence[N] = {};

    // Set the sequence from 1..N
    for (i32 i = 0; i < N; i++) {
        sequence[i] = u8(i);
    }

    for (i32 i = 0; i < N; i++) {
        u8 buf[N] = {};
        core::memcopy(buf, sequence, addr_size(i)); // copy the sequence into the buffer to i
        for (i32 j = 0; j < i; j++) {
            // Assert that the first i bytes are the same as the sequence
            Assert(buf[j] == j);
        }
        for (i32 j = i; j < N; j++) {
            // Assert that the rest are all zeroes
            Assert(buf[j] == 0);
        }
    }

    return 0;
}

i32 memsetTest() {
    constexpr i32 N = 20;
    for (i32 i = 0; i < N; i++) {
        u8 buf[N] = {};
        core::memset(buf, 7, addr_size(i)); // set the first i bytes to 7
        for (i32 j = 0; j < i; j++) {
            // Assert that the first i bytes are 7
            Assert(buf[j] == 7);
        }
        for (i32 j = i; j < N; j++) {
            // Assert that the rest are all zeroes
            Assert(buf[j] == 0);
        }
    }

    return 0;
}

i32 memcmpTest() {
    struct TestCase {
        const char* a;
        const char* b;
        addr_size n;
        enum { positive = 1, negative = -1, zero = 0 } expected;
    };

    constexpr TestCase cases[] = {
        { "", "", 0, TestCase::zero },
        { "asdzxcasd", "", 0, TestCase::zero },
        { "abc", "abc", 3, TestCase::zero },
        { "abc", "abd", 3, TestCase::negative },
        { "abd", "abc", 3, TestCase::positive },
        { "abc123", "abc000", 3, TestCase::zero },
        { "abc123", "abc000", 4, TestCase::positive },
    };

    executeTestTable("memcmpTest failed at index: ", cases, [](auto& c, const char* cErr) {
        switch (c.expected) {
            case 1:  Assert(core::memcmp(c.a, c.b, c.n) > 0, cErr);  break;
            case -1: Assert(core::memcmp(c.a, c.b, c.n) < 0, cErr);  break;
            case 0:  Assert(core::memcmp(c.a, c.b, c.n) == 0, cErr); break;
        }
    });

    return 0;
}

i32 memfillTest() {
    struct A {
        i32 a;
        u64 b;
        u8 c;
    };

    A Arr[10] = {};

    core::memfill(Arr, 10, A{ 1, 2, 3 });

    for (auto& a : Arr) {
        Assert(a.a == 1);
        Assert(a.b == 2);
        Assert(a.c == 3);
    }

    return 0;
}

i32 runMemTestsSuite() {
    RunTest(alignTest);
    RunTest(swapBytesTest);
    RunTest(memcopyTest);
    RunTest(memsetTest);
    RunTest(memcmpTest);
    RunTest(memfillTest)

    return 0;
}

constexpr i32 runCompiletimeMemTestsSuite() {
    RunTestCompileTime(alignTest);

    return 0;
}
