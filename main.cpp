#include <app.h>

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
        Application::shutdown();
    };

    if (auto res = Application::start(); res.hasErr()) {
        logFatal(res.err().toCStr());
        return -1;
    }

    return 0;
}
