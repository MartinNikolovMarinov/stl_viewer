#include "t-index.h"

i32 rndSignedIntegersTest() {
    struct TestCase { i32 min; i32 max; i32 itterCount; };

    constexpr i32 testCount = 12;
    TestCase testTable[testCount];

    testTable[0] = { 0, 1000, 5000 };
    testTable[1] = { -1000, 0, 5000 };
    testTable[2] = { 10, 11, 5000 };
    testTable[3] = { 15, 15, 5000 };
    testTable[4] = { -11, -10, 5000 };
    testTable[5] = { -19, -19, 5000 };
    testTable[6] = { -1000, 1000, 5000 };
    testTable[7] = { MIN_I32, MAX_I32, 5000 };
    testTable[8] = { MAX_I32 - 500, MAX_I32, 5000 };
    testTable[9] = { MIN_I32, MIN_I32 + 1, 5000 };
    testTable[10] = { MIN_I32, MIN_I32, 5000 };
    testTable[11] = { MAX_I32, MAX_I32, 5000 };

    for (i32 i = 0; i < testCount; i++) {
        for (i32 j = 0; j < testTable[i].itterCount; j++) {
            i32 v = core::rndI32(testTable[i].min, testTable[i].max);
            Assert(v >= testTable[i].min);
            Assert(v <= testTable[i].max);
        }
    }

    return 0;
}

i32 rndRawStrTest() {
    struct TestCase { addr_size size; i32 itterCount; };

    constexpr i32 testCount = 1;
    TestCase testTable[testCount];
    constexpr i32 max = 5000;
    testTable[0] = { 5, max - 1 }; // -1 for null terminator
    char buf[max];

    for (i32 i = 0; i < testCount; i++) {
        for (i32 j = 0; j < testTable[i].itterCount; j++) {
            core::rndCptr(buf, testTable[i].size);
            buf[max - 1] = core::term_char;
        }
    }

    return 0;
}

i32 runRndTestsSuite() {
    core::rndInit();

    RunTest(rndSignedIntegersTest);
    RunTest(rndRawStrTest);

    return 0;
}
