#include "t-index.h"

PRAGMA_WARNING_PUSH

DISABLE_MSVC_WARNING(4127) // Conditional expression is constant. I don't care here.

constexpr i32 fillStaticArrTest() {
    {
        core::SArr<i32, 4> arr(3);

        Assert(arr.cap() == 4);
        Assert(arr.len() == 3);
        Assert(arr[0] == 0);
        Assert(arr[1] == 0);
        Assert(arr[2] == 0);

        arr.fill(1, 0, 3);

        Assert(arr.cap() == 4);
        Assert(arr.len() == 3);
        Assert(arr[0] == 1);
        Assert(arr[1] == 1);
        Assert(arr[2] == 1);
    }

    {
        core::SArr<i32, 4> arr(3);

        arr.fill(1, 1, arr.len());

        Assert(arr.cap() == 4);
        Assert(arr.len() == 3);
        Assert(arr[0] == 0);
        Assert(arr[1] == 1);
        Assert(arr[2] == 1);
    }

    {
        core::SArr<i32, 4> arr(3);

        arr.fill(1, 1, 50);

        Assert(arr.cap() == 4);
        Assert(arr.len() == 4);
        Assert(arr[0] == 0);
        Assert(arr[1] == 1);
        Assert(arr[2] == 1);
        Assert(arr[3] == 1);
    }

    {
        core::SArr<i32, 4> arr(3);

        arr.fill(1, 1, 2);

        Assert(arr.cap() == 4);
        Assert(arr.len() == 3);
        Assert(arr[0] == 0);
        Assert(arr[1] == 1);
        Assert(arr[2] == 0);
    }

    return 0;
}

constexpr i32 appendStaticArrTest() {
    core::SArr<i32, 4> arr;

    Assert(arr.cap() == 4);
    Assert(arr.len() == 0);
    Assert(arr.empty());

    arr.append(1);
    Assert(arr.len() == 1);
    arr.append(2);
    Assert(arr.len() == 2);
    arr.append(3);
    Assert(arr.len() == 3);
    arr.append(4);
    Assert(arr.len() == 4);

    Assert(!arr.empty());

    Assert(arr[0] == 1);
    Assert(arr.at(0) == 1);
    Assert(arr[1] == 2);
    Assert(arr.at(1) == 2);
    Assert(arr[2] == 3);
    Assert(arr.at(2) == 3);
    Assert(arr[3] == 4);
    Assert(arr.at(3) == 4);

    Assert(arr.first() == 1);
    Assert(arr.last() == 4);

    arr.clear();
    Assert(arr.len() == 0);
    Assert(arr.empty());

    arr.append(1);
    Assert(arr.len() == 1);
    arr.append(2);
    Assert(arr.len() == 2);
    arr.append(3);
    Assert(arr.len() == 3);
    arr.append(4);
    Assert(arr.len() == 4);
    Assert(!arr.empty());

    return 0;
}

constexpr i32 copyStaticArrTest() {
    core::SArr<i32, 4> arr1;
    arr1.append(1);
    arr1.append(2);
    arr1.append(3);
    arr1.append(4);

    core::SArr<i32, 4> arr2;
    arr2.append(5);
    arr2.append(6);
    arr2.append(7);
    arr2.append(8);

    arr1 = arr2.copy();

    Assert(arr1.len() == 4);
    Assert(arr1[0] == 5);
    Assert(arr1[1] == 6);
    Assert(arr1[2] == 7);
    Assert(arr1[3] == 8);

    core::SArr<core::SArr<i32, 4>, 4> arr3;

    arr3.append(arr1.copy());
    arr3.append(arr2.copy());

    Assert(arr3.len() == 2);

    Assert(arr3[0].len() == 4);
    Assert(arr3[0][0] == 5);
    Assert(arr3[0][1] == 6);
    Assert(arr3[0][2] == 7);
    Assert(arr3[0][3] == 8);

    Assert(arr3[1].len() == 4);
    Assert(arr3[1][0] == 5);
    Assert(arr3[1][1] == 6);
    Assert(arr3[1][2] == 7);
    Assert(arr3[1][3] == 8);

    return 0;
}

constexpr i32 runStaticArrRemoveTest() {
    core::SArr<i32, 4> arr;
    arr.append(1);
    arr.append(2);
    arr.append(3);
    arr.append(4);

    Assert(arr.len() == 4);
    Assert(arr[0] == 1);
    Assert(arr[1] == 2);
    Assert(arr[2] == 3);
    Assert(arr[3] == 4);

    arr.remove(arr.len() - 1);

    Assert(arr.len() == 3);
    Assert(arr[0] == 1);
    Assert(arr[1] == 2);
    Assert(arr[2] == 3);

    arr.remove(1);

    Assert(arr.len() == 2);
    Assert(arr[0] == 1);
    Assert(arr[1] == 3);

    arr.remove(0);

    Assert(arr.len() == 1);
    Assert(arr[0] == 3);

    arr.remove(0);

    Assert(arr.len() == 0);

    return 0;
}

i32 runStaticArrTestsSuite() {
    RunTest(fillStaticArrTest);
    RunTest(appendStaticArrTest);
    RunTest(copyStaticArrTest);
    RunTest(runStaticArrRemoveTest);

    return 0;
}

constexpr i32 runCompletimeStaticArrTestsSuite() {
    RunTestCompileTime(fillStaticArrTest);
    RunTestCompileTime(appendStaticArrTest);
    RunTestCompileTime(copyStaticArrTest);
    RunTestCompileTime(runStaticArrRemoveTest);

    return 0;
}

PRAGMA_WARNING_POP
