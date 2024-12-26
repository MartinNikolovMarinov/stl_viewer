#include <app.h>

#include "./sandbox.h"

i32 main() {
    ApplicationInfo appInfo;
    appInfo.windowTitle = "Example Application";
    appInfo.initWindowHeight = 800;
    appInfo.initWindowWidth = 600;

    if (auto res = Application::init(appInfo); res.hasErr()) {
        logFatal(res.err().toCStr());
        return -1;
    }
    defer {
        Application::shutdown();
    };

    if (auto res = Application::start(); res.hasErr()) {
        logFatal(res.err().toCStr());
        return -1;
    }

    return 0;
}
