#include "t-index.h"

PRAGMA_WARNING_PUSH

DISABLE_MSVC_WARNING(4127) // Conditional expression is constant. I don't care here.

i32 expectedBasicCaseTest() {
    core::expected<i32, const char*> e1(10);
    Assert(e1.hasValue());
    Assert(!e1.hasErr());
    Assert(e1.value() == 10);
    Assert(ValueOrDie(e1) == 10);

    core::expected<i32, const char*> e2(core::unexpected("bad"));
    Assert(!e2.hasValue());
    Assert(e2.hasErr());
    Assert(core::cptrEq(e2.err(), "bad", core::cptrLen(e2.err())));

    return 0;
}

i32 expectedSizeofTest() {
    struct TestStruct {
        u64 a;
        u32 b;
        u16 c;
        i8 d;
        char test[18];
    };

    core::expected<u64, TestStruct> e1(1);
    core::expected<u64, TestStruct> e2(core::unexpected(TestStruct{1,2,3,4,"123"}));
    core::expected<TestStruct, u64> e3(TestStruct{1,2,3,4,"123"});
    core::expected<TestStruct, u64> e4(core::unexpected(u64(1)));

    Assert(sizeof(e1) == sizeof(e2));
    Assert(sizeof(e1) == sizeof(e3));
    Assert(sizeof(e1) == sizeof(e4));

    return 0;
}

i32 expectedWithSameTypeTest() {
    core::expected<i32, i32> e1(10);
    Assert(e1.hasValue());
    Assert(!e1.hasErr());
    Assert(e1.value() == 10);
    Assert(ValueOrDie(e1) == 10);

    struct TestStruct {
        u64 a;
        u32 b;
        u16 c;
        i8 d;
        char test[18];
    };

    core::expected<TestStruct, TestStruct> e2(TestStruct{1,2,3,4,"123"});
    Assert(e2.hasValue());
    Assert(!e2.hasErr());

    core::expected<TestStruct, TestStruct> e3(core::unexpected(TestStruct{1,2,3,4,"123"}));
    Assert(!e3.hasValue());
    Assert(e3.hasErr());

    return 0;
}

i32 expectedUsedInAFunctionTest() {
    constexpr const char* errMsg1 = "unexpected value less than 0";
    constexpr const char* errMsg2 = "unexpected value equals 0";

    auto f = [&](i32 v) -> core::expected<i32, const char*> {
        if (v < 0)       return core::unexpected(errMsg1);
        else if (v == 0) return core::unexpected(errMsg2);
        else             return v + 2;
    };

    Assert(ValueOrDie(f(5)) == 5 + 2);
    Assert(f(0).hasErr());
    Assert(core::cptrEq(f(0).err(), errMsg2, core::cptrLen(errMsg2)));
    Assert(f(-1).hasErr());
    Assert(core::cptrEq(f(-1).err(), errMsg1, core::cptrLen(errMsg1)));

    return 0;
}

i32 expectedWithDestructorsTest() {
    struct TestStruct {
        i32* counter;
        TestStruct(i32* _counter) : counter(_counter) {
            (*this->counter)++;
        }
        TestStruct(TestStruct&& other) : counter(other.counter) {
            (*this->counter)++;
        }
        ~TestStruct() { (*this->counter)--; }
    };

    i32 counter = 0;
    Assert(counter == 0);
    {
        core::expected<TestStruct, TestStruct> e1(TestStruct{&counter});
        Assert(counter == 1);
        {
            core::expected<TestStruct, TestStruct> e2(core::unexpected(TestStruct{&counter}));
            Assert(counter == 2);
        }
        Assert(counter == 1);
    }
    Assert(counter == 0);

    return 0;
}

constexpr i32 staticExpectedBasicCaseTest() {
    core::sexpected<i32, const char*> e1(10);
    Assert(e1.hasValue());
    Assert(!e1.hasErr());
    Assert(e1.value() == 10);
    Assert(ValueOrDie(e1) == 10);

    core::sexpected<i32, const char*> e2(core::unexpected("bad"));
    Assert(!e2.hasValue());
    Assert(e2.hasErr());
    Assert(core::cptrEq(e2.err(), "bad", core::cptrLen(e2.err())));

    return 0;
}

constexpr i32 staticExpectedSizeofTest() {
    struct TestStruct {
        u64 a;
        u32 b;
        u16 c;
        i8 d;
        char test[18];
    };

    core::sexpected<u64, TestStruct> e1(u64(1));
    core::sexpected<u64, TestStruct> e2(core::unexpected(TestStruct{1,2,3,4,"123"}));
    core::sexpected<TestStruct, u64> e3(TestStruct{1,2,3,4,"123"});
    core::sexpected<TestStruct, u64> e4(core::unexpected(u64(1)));

    Assert(sizeof(e1) == sizeof(e2));
    Assert(sizeof(e1) == sizeof(e3));
    Assert(sizeof(e1) == sizeof(e4));

    return 0;
}

constexpr i32 staticExpectedWithSameTypeTest() {
    core::sexpected<i32, i32> e1(10);
    Assert(e1.hasValue());
    Assert(!e1.hasErr());
    Assert(e1.value() == 10);
    Assert(ValueOrDie(e1) == 10);

    struct TestStruct {
        u64 a;
        u32 b;
        u16 c;
        i8 d;
        char test[18];
    };

    core::sexpected<TestStruct, TestStruct> e2(TestStruct{1,2,3,4,"123"});
    Assert(e2.hasValue());
    Assert(!e2.hasErr());

    core::sexpected<TestStruct, TestStruct> e3(core::unexpected(TestStruct{1,2,3,4,"123"}));
    Assert(!e3.hasValue());
    Assert(e3.hasErr());

    return 0;
}

constexpr i32 staticExpectedUsedInAFunctionTest() {
    constexpr const char* errMsg1 = "unexpected value less than 0";
    constexpr const char* errMsg2 = "unexpected value equals 0";

    auto f = [&](i32 v) -> core::sexpected<i32, const char*> {
        if (v < 0)       return core::unexpected(errMsg1);
        else if (v == 0) return core::unexpected(errMsg2);
        else             return v + 2;
    };

    Assert(ValueOrDie(f(5)) == 5 + 2);
    Assert(f(0).hasErr());
    Assert(core::cptrEq(f(0).err(), errMsg2, core::cptrLen(errMsg2)));
    Assert(f(-1).hasErr());
    Assert(core::cptrEq(f(-1).err(), errMsg1, core::cptrLen(errMsg1)));

    return 0;
}

PRAGMA_WARNING_POP

i32 runExpectedTestsSuite() {
    RunTest(expectedBasicCaseTest);
    RunTest(expectedSizeofTest);
    RunTest(expectedWithSameTypeTest);
    RunTest(expectedUsedInAFunctionTest);
    RunTest(expectedWithDestructorsTest);

    return 0;
}

constexpr i32 runCompiletimeExpectedTestsSuite() {
    RunTestCompileTime(staticExpectedBasicCaseTest);
    RunTestCompileTime(staticExpectedSizeofTest);
    RunTestCompileTime(staticExpectedWithSameTypeTest);
    RunTestCompileTime(staticExpectedUsedInAFunctionTest);

    return 0;
}
