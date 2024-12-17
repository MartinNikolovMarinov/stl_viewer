#include <app.h>

i32 main() {
    constexpr const char* windowTitle = "Vulkan Metal Surface Example";
    constexpr i32 windowWidth = 800, windowHeight = 600;

    if (auto err = Platform::init(windowTitle, windowWidth, windowHeight); !Platform::isOk(err)) {
        const char* errMsg = Platform::errToCStr(err);
        std::cout << "Platform init failed: " << errMsg << std::endl;
        return -1;
    }

    if (auto err = Renderer::init(); err.hasErr()) {
        const char* errMsg = Renderer::errToCStr(err.err());
        std::cout << "Platform init failed: " << errMsg << std::endl;
        return -1;
    }

    if (auto err = Platform::start(); !Platform::isOk(err)) {
        const char* errMsg = Platform::errToCStr(err);
        std::cout << "Error: " << errMsg << std::endl;
        return -1;
    }

    return 0;
}
