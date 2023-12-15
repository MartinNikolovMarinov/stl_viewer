#include "../t-index.h"

i32 basicStdAllocatorCaseTest() {
    Assert(core::cptrLen(core::StdAllocator::allocatorName()) > 0);

    core::StdAllocator::init(nullptr);

    {
        u8* data = reinterpret_cast<u8*>(core::StdAllocator::alloc(4));
        Assert(data != nullptr);
        core::StdAllocator::free(data);
        core::StdAllocator::usedMem(); // should not crash
    }

    {
        struct TestStruct {
            i32 a;
            f32 b;

            TestStruct() : a(0), b(0.0f) {}
            TestStruct(i32 _a, f32 _b) : a(_a), b(_b) {}
        };

        {
            TestStruct* ts = core::StdAllocator::construct<TestStruct>(42, 0.1f);
            Assert(ts != nullptr);
            Assert(ts->a == 42);
            Assert(ts->b == 0.1f);
            core::StdAllocator::free(ts);
        }
        {
            TestStruct* ts = reinterpret_cast<TestStruct*>(core::StdAllocator::calloc(1, sizeof(TestStruct)));
            Assert(ts != nullptr);
            Assert(ts->a == 0);
            Assert(ts->b == 0.0f);
            core::StdAllocator::free(ts);
        }
    }

    return 0;
}

i32 onOomStdAllocatorTest() {
    static i32 testOOMCount = 0;

    core::StdAllocator::init([](void*) { testOOMCount++; });

    [[maybe_unused]] void* data = core::StdAllocator::alloc(0x7fffffffffffffff);
    Assert(testOOMCount > 0);


    return 0;
}

i32 runStdAllocatorTestsSuite() {
    RunTest(basicStdAllocatorCaseTest);
    RunTest(onOomStdAllocatorTest);

    return 0;
}
