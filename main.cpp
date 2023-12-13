#include <stlv.h>

using namespace stlv;

i32 main() {
    stlv::AppCreateInfo createInfo;
    createInfo.startPosX = 0;
    createInfo.startPosY = 0;
    createInfo.startWidth = 860;
    createInfo.startHeight = 1048;
    createInfo.title = "Hello World";

    AppInstance inst;
    if (isErr(createApp(core::move(createInfo), inst))) {
        fmt::print(stderr, "Failed to create app instance.\n");
        return -1;
    }

    if (isErr(runApp(inst))) {
        fmt::print(stderr, "Application run failed.\n");
        return -2;
    }

    return 0;
}

