#include "t-index.h"

PRAGMA_WARNING_PUSH

DISABLE_MSVC_WARNING(4127) // Conditional expression is constant. I don't care here.

i32 findAlgorithmTest() {
    {
        constexpr addr_size BUFF_LEN = 5;
        i32 arr[BUFF_LEN] = {1, 2, 3, 4, 5};
        addr_off found = core::find(arr, BUFF_LEN, [](i32 elem, addr_off) -> bool { return elem == 3; });
        Assert(found == 2);
        found = core::find(arr, BUFF_LEN, [] (i32 elem, addr_off) -> bool { return elem == 6; });
        Assert(found == -1);
    }
    {
        struct TestStruct {
            i32 a;
            i32 b;
        };

        core::Arr<TestStruct> arr;
        arr.append(TestStruct{1, 2}).append(TestStruct{3, 4}).append(TestStruct{5, 6});
        addr_off found = core::find(arr, [](const TestStruct& elem, addr_off) -> bool { return elem.a == 3; });
        Assert(found == 1);
        found = core::find(arr, [] (const TestStruct& elem, addr_off) -> bool { return elem.a == 6; });
        Assert(found == -1);
    }

    return 1;
}

i32 basicAppendUniqueTest() {
    {
        core::Arr<i32> arr;

        auto eqFn = [](const i32& curr, addr_off, const i32& el) -> bool { return curr == el; };

        core::appendUnique(arr, 1, eqFn);
        core::appendUnique(arr, 2, eqFn);
        core::appendUnique(arr, 3, eqFn);
        core::appendUnique(arr, 1, eqFn);

        Assert(arr.len() == 3);
        Assert(arr[0] == 1);
        Assert(arr[1] == 2);
        Assert(arr[2] == 3);
    }

    {
        struct TestStruct {
            i32 a;
            i32 b;
        };

        core::Arr<TestStruct> arr;

        auto eqFn = [](const TestStruct& curr, addr_off, const TestStruct& el) -> bool { return curr.a == el.a; };

        core::appendUnique(arr, TestStruct{1, 2}, eqFn);
        core::appendUnique(arr, TestStruct{3, 4}, eqFn);
        core::appendUnique(arr, TestStruct{5, 6}, eqFn);
        core::appendUnique(arr, TestStruct{1, 2}, eqFn);

        Assert(arr.len() == 3);
        Assert(arr[0].a == 1);
        Assert(arr[0].b == 2);
        Assert(arr[1].a == 3);
        Assert(arr[1].b == 4);
        Assert(arr[2].a == 5);
        Assert(arr[2].b == 6);
    }

    return 1;
}

constexpr i32 constFindAlgorithmTest() {
    {
        struct TestStruct {
            i32 a;
            i32 b;
        };

        core::SArr<TestStruct, 3> staticArr;
        staticArr.append(TestStruct{1, 2}).append(TestStruct{3, 4}).append(TestStruct{5, 6});
        auto found = core::find(staticArr, [](const TestStruct& elem, addr_off) -> bool { return elem.a == 3; });
        Assert(found == 1);
        found = core::find(staticArr, [] (const TestStruct& elem, addr_off) -> bool { return elem.a == 6; });
        Assert(found == -1);
    }

    return 1;
}

constexpr i32 constBasicAppendUniqueTest() {
    core::SArr<i32, 5> staticArr;

    auto eqFn = [](const i32& curr, addr_off, const i32& el) -> bool { return curr == el; };

    core::appendUnique(staticArr, 1, eqFn);
    core::appendUnique(staticArr, 2, eqFn);
    core::appendUnique(staticArr, 3, eqFn);
    core::appendUnique(staticArr, 1, eqFn);

    Assert(staticArr.len() == 3);
    Assert(staticArr[0] == 1);
    Assert(staticArr[1] == 2);
    Assert(staticArr[2] == 3);

    return 0;
}

i32 runAlgorithmsTestsSuite() {
    RunTest(findAlgorithmTest);
    RunTest(basicAppendUniqueTest);

    return 0;
}

constexpr i32 runCompiletimeAlgorithmTestsSuite() {
    RunTestCompileTime(constFindAlgorithmTest);
    RunTestCompileTime(constBasicAppendUniqueTest);

    return 0;
}

PRAGMA_WARNING_POP
