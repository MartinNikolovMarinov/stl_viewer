#include <app.h>
#include <platform.h>
#include <renderer.h>
#include <user_input.h>

#include <iostream> // TODO2: Do I want to use c++ iostream even just for assertions it kinda sucks.

using PlatformError::Type::FAILED_TO_INITIALIZE_CORE_LOGGER;

namespace {

bool g_appIsRunning = false;

void assertHandler(const char* failedExpr, const char* file, i32 line, const char* funcName, const char* errMsg);

}

bool Application::isRunning() {
    return g_appIsRunning;
}

core::expected<AppError> Application::init(const ApplicationInfo& appInfo) {
    core::initProgramCtx(assertHandler, nullptr);
    if (!core::initLogger(core::LoggerCreateInfo())) {
        return core::unexpected(createPltErr(FAILED_TO_INITIALIZE_CORE_LOGGER,
                                "Failed to initialize core logger"));
    }
    core::setLoggerLevel(core::LogLevel::L_TRACE);
    #if defined(OS_WIN) && OS_WIN == 1
    core::useAnsiInLogger(false);
    #endif

    const char* title = appInfo.windowTitle;
    i32 w = appInfo.initWindowHeight;
    i32 h = appInfo.initWindowWidth;

    if (auto err = Platform::init(title, w, h); !err.isOk()) {
        return core::unexpected(err);
    }
    logInfo("Platform initialized SUCCESSFULLY");

    Platform::registerWindowCloseCallback([]() {
        logInfo("Closing Application!");
        g_appIsRunning = false;
    });
    Platform::registerWindowResizeCallback([](i32 w, i32 h) {
        logTrace("EVENT: WINDOW_RESIZE (w=%d, h=%d)", w, h);
    });
    Platform::registerWindowFocusCallback([](bool focus) {
        if (focus) logTrace("EVENT: WINDOW_FOCUS_GAINED");
        else       logTrace("EVENT: WINDOW_FOCUS_LOST");
    });
    Platform::registerKeyCallback([](bool isPress, i32 vkcode, i32 scancode, KeyboardModifiers mods) {
        logTrace("EVENT: KEY_%s (vkcode=%d, scancode=%d, mods=%s)",
                 isPress ? "PRESS" : "RELEASE", vkcode, scancode, keyModifiersToCptr(mods));
    });
    Platform::registerMouseClickCallback([](bool isPress, MouseButton button, i32 x, i32 y, KeyboardModifiers mods) {
        logTrace("EVENT: MOUSE_%s (button=%d, x=%d, y=%d, mods=%s)",
                 isPress ? "PRESS" : "RELEASE", button, x, y, keyModifiersToCptr(mods));
    });
    Platform::registerMouseMoveCallback([](i32 x, i32 y) {
        // NOTE: Very noisy.
        // logTrace("EVENT: MOUSE_MOVE (x=%d, y=%d)", x, y);
    });
    Platform::registerMouseScrollCallback([](MouseScrollDirection direction, i32 x, i32 y) {
        logTrace("EVENT: MOUSE_SCROLL (direction=%d, x=%d, y=%d)", direction, x, y);
    });
    Platform::registerMouseEnterOrLeaveCallback([](bool enter) {
        if (enter) logTrace("EVENT: MOUSE_ENTER");
        else       logTrace("EVENT: MOUSE_LEAVE");
    });

    if (auto res = Renderer::init(); res.hasErr()) {
        return res;
    }
    logInfo("Renderer initialized SUCCESSFULLY");

    return {};
}

core::expected<AppError> Application::start() {
    g_appIsRunning = true;
    while (Application::isRunning()) {
        if (auto err = Platform::pollEvents(true); !err.isOk()) {
            return core::unexpected(err);
        }
    }

    return {};
}

void Application::shutdown() {
    // TODO2: I probably need to handle termination signals at some point ? At least interupts like SIGINT (ctrl-C)
    Platform::shutdown();
}

namespace {

void assertHandler(const char* failedExpr, const char* file, i32 line, const char* funcName, const char* errMsg) {
    constexpr u32 stackFramesToSkip = 2;
    constexpr addr_size stackTraceBufferSize = core::CORE_KILOBYTE * 8;
    char trace[stackTraceBufferSize] = {};
    addr_size traceLen = 0;
    bool ok = core::stacktrace(trace, stackTraceBufferSize, traceLen, 200, stackFramesToSkip);

    std::cout << ANSI_RED_START() << ANSI_BOLD_START()
                << "[ASSERTION]:\n  [EXPR]: " << failedExpr
                << "\n  [FUNC]: " << funcName
                << "\n  [FILE]: " << file << ":" << line
                << "\n  [MSG]: " << (errMsg ? errMsg : "")
                << ANSI_RESET()
                << std::endl;
    std::cout << ANSI_BOLD_START() << "[TRACE]:\n" << trace << ANSI_RESET() << std::endl;

    if (!ok) {
        std::cout << ANSI_RED_START() << ANSI_BOLD_START()
                << "Failed to take full stacktrace. Consider resizing the stacktrace buffer size!"
                << ANSI_RESET()
                << std::endl;
    }

    throw std::runtime_error("Assertion failed!");
};

} // namespace
