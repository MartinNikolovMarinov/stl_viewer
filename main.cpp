#include <app.h>

#include "./tools/sandbox/sandbox.h"

i32 main() {
    ApplicationInfo appInfo = {};
    appInfo.windowTitle = "Example Application";
    appInfo.appName = "STL Viewer";
    appInfo.initWindowHeight = 1280;
    appInfo.initWindowWidth = 720;

    if (auto res = Application::init(appInfo); res.hasErr()) {
        logFatal(res.err().toCStr());
        return -1;
    }
    defer {
        // TODO2:
        // In the final product I should probably just avoid shutdown, because it can be rather slow on performance.
        // It does quite a bit and most of it is not necessary, since we just want to kill the process.
        // For now I am keeping it to ensure proper resource release and proper Vulkan spec adherence!
        Application::shutdown();
    };

    if (auto res = Application::start(); res.hasErr()) {
        logFatal(res.err().toCStr());
        return -1;
    }

    return 0;
}
