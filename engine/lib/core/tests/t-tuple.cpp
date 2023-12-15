#include "t-index.h"

struct A {
    i32 a;
    f64 b;

    constexpr A(i32 _a, f64 _b) : a(_a), b(_b) {}

    constexpr bool operator==(const A& other) const { return a == other.a && b == other.b; }
    constexpr bool operator!=(const A& other) const { return !(*this == other); }
};

constexpr i32 runTupleArgumentIncrementTest() {
    auto t1 = core::createTuple(1, A{2, 3.0}, u64(6));
    auto t2 = t1;

    Assert(t1.get<0>() == t2.get<0>());
    Assert(t1.get<1>() == t2.get<1>());
    Assert(t1.get<2>() == t2.get<2>());

    Assert(t1.get<2>() == 6);
    auto& ref = t1.get<2>();
    ref++;
    Assert(t1.get<2>() == 7);
    auto& [a, b, c] = t1;
    c++;
    Assert(t1.get<2>() == 8);

    Assert(t1.get<0>() == t2.get<0>());
    Assert(t1.get<1>() == t2.get<1>());
    Assert(t1.get<2>() != t2.get<2>());

    return 0;
}

PRAGMA_WARNING_PUSH

DISABLE_MSVC_WARNING(4127) // Disable conditional expression is constant. This is intentional here.

constexpr i32 runCreateTuplesOfDifferentSizesTest() {
    {
        auto t = core::createTuple(1, 2);
        Assert(t.len == 2);
        Assert(t.get<0>() == 1);
        Assert(t.get<1>() == 2);
        auto [first, second] = t;
        Assert(first == 1);
        Assert(second == 2);
    }
    {
        auto t = core::createTuple(1, 2, 3);
        Assert(t.len == 3);
        Assert(t.get<0>() == 1);
        Assert(t.get<1>() == 2);
        Assert(t.get<2>() == 3);
        auto [first, second, third] = t;
        Assert(first == 1);
        Assert(second == 2);
        Assert(third == 3);
    }
    {
        auto t = core::createTuple(1, 2, 3, 4);
        Assert(t.len == 4);
        Assert(t.get<0>() == 1);
        Assert(t.get<1>() == 2);
        Assert(t.get<2>() == 3);
        Assert(t.get<3>() == 4);
        auto [first, second, third, fourth] = t;
        Assert(first == 1);
        Assert(second == 2);
        Assert(third == 3);
        Assert(fourth == 4);
    }

    return 0;
}

PRAGMA_WARNING_POP

i32 runTupleTestsSuite() {
    RunTest(runTupleArgumentIncrementTest);
    RunTest(runCreateTuplesOfDifferentSizesTest);
    return 0;
}

constexpr i32 runCompiletimeTupleTestsSuite() {
    RunTestCompileTime(runTupleArgumentIncrementTest);
    RunTestCompileTime(runCreateTuplesOfDifferentSizesTest);
    return 0;
}
