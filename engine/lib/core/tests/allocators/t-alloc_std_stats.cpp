#include "../t-index.h"

i32 statsAllocatorBasicCaseTest() {
    core::StdStatsAllocator::init(nullptr);

    {
        void* data = core::StdStatsAllocator::alloc(4);
        Assert(data != nullptr);
        Assert(core::StdStatsAllocator::usedMem() == 4);
    }

    {
        void* data = core::StdStatsAllocator::alloc(5);
        Assert(data != nullptr);
        Assert(core::StdStatsAllocator::usedMem() == 9);
    }

    {
        void* data = core::StdStatsAllocator::alloc(6);
        Assert(data != nullptr);
        Assert(core::StdStatsAllocator::usedMem() == 15);
    }

    {
        void* data = core::StdStatsAllocator::alloc(10);
        Assert(data != nullptr);
        Assert(core::StdStatsAllocator::usedMem() == 25);
        core::StdStatsAllocator::free(data);
        Assert(core::StdStatsAllocator::usedMem() == 15);
    }

    {
        void* data = core::StdStatsAllocator::calloc(10, sizeof(char));
        Assert(data != nullptr);
        Assert(core::StdStatsAllocator::usedMem() == 25);
        char buff[10] = {};
        Assert(core::memcmp(data, buff, 10) == 0, "Memory should be zeroed out.");
        core::StdStatsAllocator::free(data);
        Assert(core::StdStatsAllocator::usedMem() == 15);
    }

    core::StdStatsAllocator::clear();
    Assert(core::StdStatsAllocator::usedMem() == 0);

    return 0;
}

i32 statsAllocatorWithConstructTest() {
    core::StdStatsAllocator::init(nullptr);

    struct TestStruct {
        i32 a;
        f32 b;

        TestStruct() : a(0), b(0.0f) {}
        TestStruct(i32 _a, f32 _b) : a(_a), b(_b) {}
    };

    TestStruct* ts = nullptr;
    ts = core::StdStatsAllocator::construct<TestStruct>(42, 0.1f);
    Assert(ts != nullptr);
    Assert(ts->a == 42);
    Assert(ts->b == 0.1f);
    Assert(core::StdStatsAllocator::usedMem() == sizeof(TestStruct));

    core::StdStatsAllocator::free(ts);
    Assert(core::StdStatsAllocator::usedMem() == 0);

    constexpr i32 testCount = 100;
    i32* tcases[testCount]; core::memset(tcases, 0, sizeof(tcases));
    for (i32 i = 0; i < testCount; ++i) {
        tcases[i] = core::StdStatsAllocator::construct<i32>(i);
    }

    Assert(core::StdStatsAllocator::usedMem() == sizeof(i32) * testCount);
    core::StdStatsAllocator::clear();
    Assert(core::StdStatsAllocator::usedMem() == 0);

    // test case with different sizes
    i32* a = nullptr;
    f64* b = nullptr;
    TestStruct* c = nullptr;
    a = core::StdStatsAllocator::construct<i32>(1);
    b = core::StdStatsAllocator::construct<f64>(2.5);
    c = core::StdStatsAllocator::construct<TestStruct>(3, 3.5f);

    Assert(core::StdStatsAllocator::usedMem() == sizeof(i32) + sizeof(f64) + sizeof(TestStruct));
    Assert(*a == 1);
    Assert(*b == 2.5);
    Assert((*c).a == 3);
    Assert((*c).b == 3.5f);

    core::StdStatsAllocator::free(a);
    Assert(core::StdStatsAllocator::usedMem() == sizeof(f64) + sizeof(TestStruct));
    core::StdStatsAllocator::free(b);
    Assert(core::StdStatsAllocator::usedMem() == sizeof(TestStruct));
    core::StdStatsAllocator::free(c);
    Assert(core::StdStatsAllocator::usedMem() == 0);

    return 0;
}

i32 runStdStatsAllocatorTestsSuite() {
    RunTest(statsAllocatorBasicCaseTest);
    RunTest(statsAllocatorWithConstructTest);

    return 0;
}
