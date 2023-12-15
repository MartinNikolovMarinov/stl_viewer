#include "../t-index.h"

i32 basicBumpAllocatorTest() {
    Assert(core::cptrLen(core::BumpAllocator::allocatorName()) > 0);

    constexpr addr_size BUFF_SIZE = 254;
    char buff[BUFF_SIZE];
    core::BumpAllocator::init(nullptr, buff, BUFF_SIZE);

    {
        u8* data = reinterpret_cast<u8*>(core::BumpAllocator::alloc(4));
        Assert(data != nullptr);
        Assert(core::BumpAllocator::usedMem() == core::align(4));

        data[0] = 0;
        data[1] = 1;
        data[2] = 2;
        data[3] = 3;

        Assert(buff[0] == 0);
        Assert(buff[1] == 1);
        Assert(buff[2] == 2);
        Assert(buff[3] == 3);

        core::BumpAllocator::free(data); // should not crash

        core::BumpAllocator::clear();
        Assert(core::BumpAllocator::usedMem() == 0);
    }

    {
        struct TestStruct {
            i32 a;
            f32 b;

            TestStruct() : a(0), b(0.0f) {}
            TestStruct(i32 _a, f32 _b) : a(_a), b(_b) {}
        };

        {
            TestStruct* ts = core::BumpAllocator::construct<TestStruct>(42, 0.1f);
            Assert(ts != nullptr);
            Assert(ts->a == 42);
            Assert(ts->b == 0.1f);
        }
        Assert(core::BumpAllocator::usedMem() == core::align(sizeof(TestStruct)));
        {
            TestStruct* ts = reinterpret_cast<TestStruct*>(core::BumpAllocator::calloc(1, sizeof(TestStruct)));
            Assert(ts != nullptr);
            Assert(ts->a == 0);
            Assert(ts->b == 0.0f);
        }
        Assert(core::BumpAllocator::usedMem() == core::align(sizeof(TestStruct)) * 2);

        core::BumpAllocator::clear();
        Assert(core::BumpAllocator::usedMem() == 0);
    }

    return 0;
}

i32 onOomBumpAllocatorTest() {
    static i32 testOOMCount = 0;

    constexpr addr_size BUFF_SIZE = 254;
    char buff[BUFF_SIZE];
    core::BumpAllocator::init([](void*) { testOOMCount++; }, buff, BUFF_SIZE);

    [[maybe_unused]] void* data = core::BumpAllocator::alloc(255);
    Assert(testOOMCount > 0);

    return 0;
}

i32 runBumpAllocatorTestsSuite() {
    RunTest(basicBumpAllocatorTest);
    RunTest(onOomBumpAllocatorTest);

    return 0;
}
