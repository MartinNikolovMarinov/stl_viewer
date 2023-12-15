#include "../t-index.h"

i32 pltErrorDescribeWorksTest() {
    char buf[core::MAX_SYSTEM_ERR_MSG_SIZE] = {};
    bool ok = core::pltErrorDescribe(0, buf);
    Assert(ok, "Platform failed to describe error code 0.");

    i32 pltSpecificErrCode = 0;

#if defined(OS_WIN) && OS_WIN == 1
    pltSpecificErrCode = ERROR_FILE_NOT_FOUND;
#elif (defined(OS_LINUX) && OS_LINUX) || (defined(OS_MAC) && OS_MAC == 1)
    pltSpecificErrCode = ENOENT;
    ok = core::pltErrorDescribe(pltSpecificErrCode, buf);
    Assert(ok);
    Assert(core::cptrLen(buf) > 0);
#endif

    return 0;
}

i32 runPltErrorTestsSuite() {
    RunTest(pltErrorDescribeWorksTest);

    return 0;
}
