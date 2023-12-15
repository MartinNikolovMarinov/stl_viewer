#include "../t-index.h"

i32 getTheSystemPageSizeTest() {
    addr_size pageSize = core::getPageSize();
    Assert(pageSize > 0);
    return 0;
}

i32 allocateAndFreePagesTest() {
    addr_size pageSize = core::getPageSize();
    constexpr addr_size pageCount = 20;
    void* addr = nullptr;
    {
        auto res = core::allocPages(pageSize * pageCount);
        Assert(!res.hasErr());
        Assert(res.hasValue());
        Assert(res.value() != nullptr);
        addr = res.value();
    }
    {
        auto res = core::freePages(addr, pageSize * pageCount);
        Assert(!res.hasErr());
    }

    return 0;
}

i32 runPltPagesTestsSuite() {
    RunTest(getTheSystemPageSizeTest);
    RunTest(allocateAndFreePagesTest);

    return 0;
}
