#include "t-index.h"

i32 deferTest() {
    {
        i32 a = 0;
        {
            Assert(a == 0);
            defer { a++; };
            Assert(a == 0);

            {
                Assert(a == 0);
                defer { a++; };
                Assert(a == 0);
            }

            Assert(a == 1);
        }
        Assert(a == 2);
    }

    return 0;
}

i32 runDeferTestsSuite() {
    RunTest(deferTest);

    return 0;
}
