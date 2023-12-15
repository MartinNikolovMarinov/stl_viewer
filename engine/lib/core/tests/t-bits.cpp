#include "t-index.h"

constexpr i32 leastSignificantNBitsTest() {
    struct TestCase { u8 value; u8 bitSeq; u8 n; bool expected; };

    TestCase cases[] = {
        { 0b0, 0b0, 1, true },
        { 0b1, 0b1, 1, true },
        { 0b1, 0b0, 1, false },
        { 0b0, 0b1, 1, false },

        { 0b11, 0b11, 2, true },
        { 0b11, 0b11, 1, false }, // This is undefined behaviour, maybe I should not test for it.

        { 0b100, 0b100, 3, true },
        { 0b1001, 0b1001, 4, true },
        { 0b10011, 0b10011, 5, true },
        { 0b100111, 0b100111, 6, true },
        { 0b1001110, 0b1001110, 7, true },
        { 0b10011100, 0b10011100, 8, true },
        { 0b11111111, 0b11111111, 8, true },
        { 0b00011111, 0b11111, 5, true },
        { 0b10000001, 0b10000001, 8, true },
        { 0b01000010, 0b01000010, 8, true },
        { 0b01010101, 0b01010101, 8, true },

        { 0b11111111, 0b11101111, 8, false },
        { 0b10101111, 0b11101111, 8, false },
        { 0b11111000, 0b11111, 5, false },
    };

    executeTestTable("test case failed at index: ", cases, [](auto& c, const char* cErr) {
        bool got = core::leastSignificantNBits(c.value, c.bitSeq, c.n);
        Assert(got == c.expected, cErr);
    });

    return 0;
}

constexpr i32 mostSignificantNBistsTest() {
    struct TestCase { u8 value; u8 bitSeq; u8 n; bool expected; };

    TestCase cases[] = {
        { 0b10000000, 0b1, 1, true },

        { 0b11000000, 0b1, 1, true },
        { 0b11000000, 0b11, 2, true },

        { 0b11100000, 0b1, 1, true },
        { 0b11100000, 0b11, 2, true },
        { 0b11100000, 0b111, 3, true },

        { 0b11110000, 0b1, 1, true },
        { 0b11110000, 0b11, 2, true },
        { 0b11110000, 0b111, 3, true },
        { 0b11110000, 0b1111, 4, true },

        { 0b11111000, 0b1, 1, true },
        { 0b11111000, 0b11, 2, true },
        { 0b11111000, 0b111, 3, true },
        { 0b11111000, 0b1111, 4, true },
        { 0b11111000, 0b11111, 5, true },

        { 0b11111100, 0b1, 1, true },
        { 0b11111100, 0b11, 2, true },
        { 0b11111100, 0b111, 3, true },
        { 0b11111100, 0b1111, 4, true },
        { 0b11111100, 0b11111, 5, true },
        { 0b11111100, 0b111111, 6, true },

        { 0b11111110, 0b1, 1, true },
        { 0b11111110, 0b11, 2, true },
        { 0b11111110, 0b111, 3, true },
        { 0b11111110, 0b1111, 4, true },
        { 0b11111110, 0b11111, 5, true },
        { 0b11111110, 0b111111, 6, true },
        { 0b11111110, 0b1111111, 7, true },

        { 0b11111111, 0b1, 1, true },
        { 0b11111111, 0b11, 2, true },
        { 0b11111111, 0b111, 3, true },
        { 0b11111111, 0b1111, 4, true },
        { 0b11111111, 0b11111, 5, true },
        { 0b11111111, 0b111111, 6, true },
        { 0b11111111, 0b1111111, 7, true },
        { 0b11111111, 0b11111111, 8, true },

        { 0b10001111, 0b1000, 4, true },
        { 0b10101011, 0b101010, 6, true },
        { 0b11111101, 0b1111110, 7, true },
        { 0b11111111, 0b1111, 4, true },

        { 0b1, 0b1, 1, false },
        { 0b10, 0b1, 1, false },
        { 0b100, 0b1, 1, false },
        { 0b1000, 0b1, 1, false },
        { 0b10000, 0b1, 1, false },
        { 0b100000, 0b1, 1, false },
        { 0b1000000, 0b1, 1, false },

        { 0b101, 0b101, 3, false },
        { 0b1010, 0b101, 3, false },
        { 0b10100, 0b101, 3, false },
        { 0b101000, 0b101, 3, false },
        { 0b1010000, 0b101, 3, false },
        { 0b10100000, 0b101, 3, true },
    };

    executeTestTable("test case failed at index: ", cases, [](auto& c, const char* cErr) {
        bool got = core::mostSignificantNBits(c.value, c.bitSeq, c.n);
        Assert(got == c.expected, cErr);
    });

    return 0;
}

i32 runBitsTestsSuite() {
    RunTest(leastSignificantNBitsTest);
    RunTest(mostSignificantNBistsTest);

    return 0;
}

constexpr i32 runCompiletimeBitsTestsSuite() {
    RunTestCompileTime(leastSignificantNBitsTest);
    RunTestCompileTime(mostSignificantNBistsTest);

    return 0;
}
