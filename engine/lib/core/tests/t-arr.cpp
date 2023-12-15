#include "t-index.h"

template <typename TAllocator>
i32 initializeArrTest() {
    {
        core::Arr<i32, TAllocator> arr;
        Assert(arr.len() == 0);
        Assert(arr.cap() == 0);
        Assert(arr.data() == nullptr);
        Assert(arr.empty());
    }

    {
        core::Arr<i32, TAllocator> arr(10);
        Assert(arr.len() == 10);
        Assert(arr.cap() == 10);
        Assert(!arr.empty());

        for (addr_size i = 0; i < arr.len(); ++i) {
            Assert(arr[i] == 0);
        }
    }

    {
        core::Arr<i32, TAllocator> arr(10, 20);
        Assert(arr.len() == 10);
        Assert(arr.cap() == 20);
        Assert(!arr.empty());

        for (addr_size i = 0; i < arr.len(); ++i) {
            Assert(arr[i] == 0);
        }
    }

    {
        defer { SVCT::nextId = 0; };

        Assert(SVCT::nextId == 0);
        core::Arr<SVCT, TAllocator> arr(10);
        Assert(arr.len() == 10);
        Assert(arr.cap() == 10);
        Assert(!arr.empty());

        for (addr_size i = 0; i < arr.len(); ++i) {
            Assert(arr[i].a == i32(i));
        }
    }

    {
        defer { CT::resetAll(); };
        constexpr i32 testCount = 10;
        {
            core::Arr<CT, TAllocator> arr(testCount);
            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == 7, "Initializer did not call constructors!");
            }
            Assert(CT::totalCtorsCalled() == testCount, "Initializer did not call the exact number of constructors!");
            Assert(CT::defaultCtorCalled() == testCount, "Initializer did not call the exact number of copy constructors!");
            CT::resetCtors();
            {
                auto arrCpy = arr.copy();
                Assert(arrCpy.data() != arr.data());
                for (addr_size i = 0; i < arrCpy.len(); ++i) {
                    Assert(arrCpy[i].a == 7, "Copy constructor did not call constructors!");
                }
                Assert(CT::totalCtorsCalled() == testCount, "Copy constructor did not call the exact number of constructors!");
                Assert(CT::copyCtorCalled() == testCount, "Copy constructor did not call the exact number of copy constructors!");
                CT::resetCtors();
            }
            Assert(CT::dtorsCalled() == testCount, "Copy constructor did not call destructors!");
            CT::resetDtors();
        }
        Assert(CT::dtorsCalled() == testCount, "Copy constructor did not call destructors!");
        CT::resetDtors();
    }

    return 0;
}

template <typename TAllocator>
i32 moveAndCopyArrTest() {
    core::Arr<i32, TAllocator> arr(10);
    arr.fill(1, 0, arr.len());
    core::Arr<i32, TAllocator> arrCpy;
    core::Arr<i32, TAllocator> arrMoved(10);

    arrCpy = arr.copy();
    Assert(arrCpy.len() == arr.len());
    Assert(arrCpy.cap() == arr.cap());
    Assert(arrCpy.data() != arr.data());
    Assert(!arrCpy.empty());
    for (addr_size i = 0; i < arrCpy.len(); ++i) {
        Assert(arrCpy[i] == arr[i]);
    }

    auto arrCpy2 = arr.copy();
    Assert(arrCpy2.len() == arr.len());
    Assert(arrCpy2.cap() == arr.cap());
    Assert(arrCpy2.data() != arr.data());
    Assert(!arrCpy2.empty());
    for (addr_size i = 0; i < arrCpy2.len(); ++i) {
        Assert(arrCpy2[i] == arr[i]);
    }

    arrMoved = core::move(arr);
    Assert(arr.len() == 0);
    Assert(arr.cap() == 0);
    Assert(arr.data() == nullptr);
    Assert(arr.empty());
    Assert(arrMoved.len() == arrCpy.len());
    Assert(arrMoved.cap() == arrCpy.cap());
    Assert(!arrMoved.empty());
    for (addr_size i = 0; i < arrMoved.len(); ++i) {
        Assert(arrMoved[i] == arrCpy[i]);
    }

    return 0;
}

template <typename TAllocator>
i32 resizeArrTest() {
    {
        core::Arr<i32, TAllocator> arr;
        Assert(arr.len() == 0);
        Assert(arr.cap() == 0);
        Assert(arr.data() == nullptr);
        Assert(arr.empty());

        arr.adjustCap(10);
        Assert(arr.len() == 0);
        Assert(arr.cap() == 10);
        Assert(arr.data() != nullptr);
        Assert(arr.empty());

        arr.adjustCap(0);
        Assert(arr.len() == 0);
        Assert(arr.cap() == 0);
        Assert(arr.data() != nullptr);
        Assert(arr.empty());
    }

    return 0;
}

template <typename TAllocator>
i32 fillArrTest() {

    // Invalid from and to values should not have any effect.
    {
        core::Arr<i32, TAllocator> arr;

        arr.fill(0, 1, 0);
        Assert(arr.len() == 0);
        Assert(arr.cap() == 0);
        Assert(arr.data() == nullptr);
        Assert(arr.empty());

        arr.fill(0, 0, 0);
        Assert(arr.len() == 0);
        Assert(arr.cap() == 0);
        Assert(arr.data() == nullptr);
        Assert(arr.empty());
    }

    // Filling an array exactly to capacity:
    {
        constexpr i32 N = 10;
        core::Arr<i32, TAllocator> arr(N);

        arr.fill(0, 0, N);
        for (addr_size i = 0; i < N; ++i) {
            Assert(arr[i] == 0);
        }

        arr.fill(1, 0, N);
        for (addr_size i = 0; i < N; ++i) {
            Assert(arr[i] == 1);
        }
    }

    // Filling an array of pointers exactly to capacity with a structure:
    {
        constexpr i32 N = 10;
        struct TestStruct {
            i32 a;
            f64 b;
        };
        core::Arr<TestStruct*, TAllocator> arr2(N);
        TestStruct t = { 1, 2.0 };

        arr2.fill(&t, 0, N);
        for (addr_size i = 0; i < arr2.len(); ++i) {
            Assert(arr2[i]->a == 1);
            Assert(arr2[i]->b == 2.0);
        }

        arr2.fill(nullptr, 0, N);
        for (addr_size i = 0; i < arr2.len(); ++i) {
            Assert(arr2[i] == nullptr);
        }
    }

    // Fill emtpy array:
    {
        core::Arr<i32, TAllocator> arr;

        arr.fill(0, 0, 10);
        Assert(arr.len() == 10);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < arr.len(); ++i) {
            Assert(arr[i] == 0);
        }
    }

    // Partial fill:
    {
        core::Arr<i32, TAllocator> arr (10);
        arr.fill(0, 0, 10);

        arr.fill(1, 0, 5);

        for (addr_size i = 0; i < 5; ++i) {
            Assert(arr[i] == 1);
        }
        for (addr_size i = 5; i < 10; ++i) {
            Assert(arr[i] == 0);
        }
    }

    // Assure proper object destructors are called:
    {
        CT::resetAll();
        defer { CT::resetAll(); };
        constexpr i32 testCount = 10;
        CT v;

        {
            core::Arr<CT, TAllocator> arr;
            arr.fill(v, 0, testCount);
            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == CT::defaultValue, "Fill did not call the default constructors.");
                arr[i].a = 8; // Modify the default should not affect the original object v.
            }
            Assert(CT::dtorsCalled() == 0, "No destructors should be called by fill in this case.");
            Assert(v.a == 7, "The value passed to the fill function was modified.");
        }
        Assert(CT::dtorsCalled() == testCount, "Destructors where not called after the array went out of scope.");
        CT::resetAll();

        {
            v.a = 9; // Fill must use this value!

            core::Arr<CT, TAllocator> arr(testCount); // Initially all elements are default constructed.

            arr.fill(v, 0, testCount / 2); // Filling half to assert that the ones that are overwritten are freed.

            for (addr_size i = 0; i < testCount / 2; ++i) {
                Assert(arr[i].a == 9, "Fill did not work.");
            }
            for (addr_size i = testCount / 2; i < arr.len(); ++i) {
                Assert(arr[i].a == CT::defaultValue, "Fill overwrote elements that it should not have.");
            }
            Assert(CT::dtorsCalled() == testCount / 2, "Exactly half of the destructors should have been called by fill.");

            arr.fill(v, testCount / 2, testCount); // Filling the rest of the array should call the remaining destructors.

            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == 9, "Fill did not work.");
            }
            Assert(CT::dtorsCalled() == testCount, "All destructors should have been called by fill.");

            CT::resetDtors();
        }
        Assert(CT::dtorsCalled() == testCount, "Destructors where not called after the array went out of scope.");
        CT::resetAll();

        {
            v.a = 7; // Fill must use this value!

            core::Arr<CT, TAllocator> arr(testCount / 2); // Initially half of the elements are default constructed.

            arr.fill(v, 0, testCount); // Filling the whole array should call only half of the destructors.

            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == 7, "Fill did not work.");
            }
            Assert(CT::dtorsCalled() == testCount / 2, "Exactly half of the destructors should have been called by fill.");

            CT::resetDtors();
        }
        Assert(CT::dtorsCalled() == testCount, "Destructors where not called after the array went out of scope.");
        CT::resetAll();

        {
            v.a = 200; // Fill must use this value!

            core::Arr<CT, TAllocator> arr(testCount); // Initially all elements are default constructed.

            arr.fill(v, 2, 5); // Filling a range of the array should call only the destructors of the overwritten elements.

            for (addr_size i = 0; i < arr.len(); ++i) {
                if (i >= 2 && i < 5) {
                    Assert(arr[i].a == 200, "Fill did not work.");
                }
                else {
                    Assert(arr[i].a == CT::defaultValue, "Fill overwrote elements that it should not have.");
                }
            }
            Assert(CT::dtorsCalled() == 3, "Exactly 3 destructors should have been called by fill.");

            CT::resetDtors();
        }
        Assert(CT::dtorsCalled() == testCount, "Destructors where not called after the array went out of scope.");
        CT::resetAll();
    }

    return 0;
}

template <typename TAllocator>
i32 appendArrTest() {
    {
        core::Arr<i32, TAllocator> arr;

        arr.append(1);
        for (addr_size i = 0; i < 1; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.append(2);
        for (addr_size i = 0; i < 2; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.clear();

        arr.append(1);
        for (addr_size i = 0; i < 1; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.append(2);
        for (addr_size i = 0; i < 2; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.append(3);
        Assert(arr.len() == 3);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < 3; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.adjustCap(2);

        arr.append(3);
        Assert(arr.len() == 3);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < 3; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.append(4);
        Assert(arr.len() == 4);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < 4; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.append(5);
        Assert(arr.len() == 5);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < 5; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }

        arr.clear();

        // Append many trivial values.
        i32 many[5] = { 1, 2, 3, 4, 5 };
        arr.append(many, 5);
        Assert(arr.len() == 5);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < 5; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }
    }

    {
        defer { CT::resetAll(); };
        CT lv;
        CT::resetCtors(); // Don't count the default ctors of the above code

        {
            core::Arr<CT, TAllocator> arr;
            arr.append(CT{});
            arr.append(lv);
            Assert(CT::copyCtorCalled() == 1);
            Assert(CT::moveCtorCalled() == 1);
            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == 7, "Append did not call the default constructors.");
            }
        }
        Assert(CT::dtorsCalled() == 3);
        CT::resetAll();

        // Testing a combination of append and adjustCap.
        {
            core::Arr<CT, TAllocator> arr;
            arr.adjustCap(1);
            Assert(arr.len() == 0);
            Assert(arr.cap() == 1);
            Assert(CT::noCtorsCalled());

            arr.append(CT{}); // calls ctor and dtor
            arr.append(lv);
            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == 7, "Append did not call the default constructors.");
            }
            Assert(arr.len() == 2);
            Assert(arr.cap() == 2);
            arr.adjustCap(0); // This adjustCap should call the destructors.
            Assert(arr.len() == 0);
            Assert(arr.cap() == 0);
            Assert(CT::dtorsCalled() == 3);
        }
        Assert(CT::dtorsCalled() == 3);
        CT::resetAll();

        // Test appending multiple values
        CT staticArr[5];
        CT::resetCtors(); // Don't count the default ctors of the above code
        {
            core::Arr<CT, TAllocator> arr;
            arr.append(staticArr, 5);
            Assert(arr.len() == 5);
            Assert(arr.cap() >= arr.len());
            Assert(CT::copyCtorCalled() == 5);
            Assert(CT::moveCtorCalled() == 0);
            Assert(CT::defaultCtorCalled() == 0);
            for (addr_size i = 0; i < arr.len(); ++i) {
                Assert(arr[i].a == 7, "Append multiple did not call the default constructors.");
            }
        }
        Assert(CT::dtorsCalled() == 5);
        CT::resetAll();
    }

    {
        // Appending arrays that are bigger than double current capacity.
        core::Arr<i32, TAllocator> arr(0, 2);
        core::Arr<i32, TAllocator> arr2;
        arr2.append(1).append(2).append(3).append(4).append(5).append(6).append(7).append(8).append(9).append(10);

        arr.append(arr2);
        Assert(arr.len() == 10);
        Assert(arr.cap() >= arr.len());
        for (addr_size i = 0; i < 10; ++i) {
            Assert(arr[i] == i32(i + 1));
            Assert(arr.at(i) == i32(i + 1));
        }
    }

    return 0;
}

template <typename TAllocator>
i32 arrayOfArraysArrTest() {
    {
        core::Arr<i32, TAllocator> arr;
        core::Arr<i32, TAllocator> arr2;
        core::Arr<i32, TAllocator> arr3;
        core::Arr<core::Arr<i32, TAllocator>, TAllocator> multi;

        arr.append(1);
        arr.append(2);
        arr.append(3);

        arr2.append(4);
        arr2.append(5);
        arr2.append(6);

        arr3.append(7);
        arr3.append(8);
        arr3.append(9);

        multi.append(arr.copy());
        multi.append(core::move(arr2));
        multi.append(core::move(arr3));

        // arr 1 should be copied
        Assert(arr.len() == 3);
        Assert(arr[0] == 1);
        Assert(arr[1] == 2);
        Assert(arr[2] == 3);

        // arr 2 and 3 should have been moved
        Assert(arr2.len() == 0);
        Assert(arr2.data() == nullptr);
        Assert(arr3.len() == 0);
        Assert(arr3.data() == nullptr);

        Assert(multi.len() == 3);
        Assert(multi[0].len() == 3);
        Assert(multi[1].len() == 3);
        Assert(multi[2].len() == 3);
        Assert(multi[0][0] == 1);
        Assert(multi[0][1] == 2);
        Assert(multi[0][2] == 3);
        Assert(multi[1][0] == 4);
        Assert(multi[1][1] == 5);
        Assert(multi[1][2] == 6);
        Assert(multi[2][0] == 7);
        Assert(multi[2][1] == 8);
        Assert(multi[2][2] == 9);
    }

    return 0;
}

template <typename TAllocator>
i32 clearArrayShouldCallDtorsTest() {
    defer { CT::resetAll(); };
    constexpr i32 testCount = 10;
    auto arr = core::Arr<CT, TAllocator>(testCount);
    arr.clear();
    Assert(CT::dtorsCalled() == testCount, "Clear should call dtors on all elements.");
    Assert(arr.cap() == testCount, "Clear should not change the capacity of the array.");

    CT::resetDtors();

    arr.clear();
    arr = core::Arr<CT, TAllocator>(testCount);
    arr.clear();
    Assert(CT::dtorsCalled() == testCount, "Clear should call dtors on all elements.");
    Assert(arr.cap() == testCount, "Clear should not change the capacity of the array.");

    return 0;
}

template <typename TAllocator>
i32 removeFromArrayTest() {

    {
        core::Arr<i32, TAllocator> arr;

        {
            arr.append(1).remove(0);
            Assert(arr.len() == 0);
            arr.clear();
        }
        {
            arr.append(1).append(2).remove(arr.len() - 1);
            Assert(arr.len() == 1);
            Assert(arr[0] == 1);
            arr.clear();
        }
        {
            arr.append(1).append(2).remove(0);
            Assert(arr.len() == 1);
            Assert(arr[0] == 2);
            arr.clear();
        }
    }

    {
        core::Arr<CT, TAllocator> arr;

        defer { CT::resetAll(); };

        CT a, b;

        {
            CT::resetDtors();
            arr.append(a).remove(0);
            Assert(CT::dtorsCalled() == 1);
            Assert(arr.len() == 0);
            arr.clear();
        }
        {
            CT::resetDtors();
            arr.append(a).append(b).remove(arr.len() - 1);
            Assert(CT::dtorsCalled() == 1);
            Assert(arr.len() == 1);
            arr.clear();
        }
        {
            CT::resetDtors();
            arr.append(a).append(b).remove(0);
            Assert(CT::dtorsCalled() == 1);
            Assert(arr.len() == 1);
            arr.clear();
        }
    }

    return 0;
}

template <typename TAllocator>
i32 resetArrayTest() {
    auto unmanaged = reinterpret_cast<u8*>(TAllocator::alloc(256));
    core::memset(unmanaged, 5, 256);

    core::Arr<u8, TAllocator> arr;
    arr.reset(unmanaged, 256); // arr becomes the owner and thus must free the memory when it goes out of scope.

    Assert(arr.len() == 256);
    Assert(arr.cap() == 256);
    Assert(arr.data() == unmanaged);

    for (addr_size i = 0; i < arr.len(); ++i) {
        Assert(arr[i] == 5);
    }

    return 0;
}

i32 runArrTestsSuite() {
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
        RunTest_DisplayMemAllocs(initializeArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(initializeArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(initializeArrTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(moveAndCopyArrTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(moveAndCopyArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(moveAndCopyArrTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(resizeArrTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(resizeArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(resizeArrTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(fillArrTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(fillArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(fillArrTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(appendArrTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(appendArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(appendArrTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(arrayOfArraysArrTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(arrayOfArraysArrTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(arrayOfArraysArrTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(clearArrayShouldCallDtorsTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(clearArrayShouldCallDtorsTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(clearArrayShouldCallDtorsTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(removeFromArrayTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(removeFromArrayTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(removeFromArrayTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(resetArrayTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(resetArrayTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(resetArrayTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }

    return 0;
}
