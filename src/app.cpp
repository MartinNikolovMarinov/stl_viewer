#include <app.h>
#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <user_input.h>

#include <iostream>

using PlatformError::Type::FAILED_TO_INITIALIZE_CORE_LOGGER;

namespace {

#if !defined(OS_WIN) || OS_WIN != 1
constexpr bool USE_ANSI_LOGGING = true;
#else
constexpr bool USE_ANSI_LOGGING = false;
#endif

bool g_appIsRunning = false;

core::expected<AppError> initCoreContext();
void registerEventHandlers();

void assertHandler(const char* failedExpr, const char* file, i32 line, const char* funcName, const char* errMsg);

}

bool Application::isRunning() {
    return g_appIsRunning;
}

core::expected<AppError> Application::init(const ApplicationInfo& appInfo) {
    if (auto res = initCoreContext(); res.hasErr()) {
        return res;
    }

    const char* title = appInfo.windowTitle;
    i32 w = appInfo.initWindowHeight;
    i32 h = appInfo.initWindowWidth;
    if (auto err = Platform::init(title, w, h); !err.isOk()) {
        return core::unexpected(err);
    }
    logInfo("Platform initialized SUCCESSFULLY");

    registerEventHandlers();

    logSectionTitleInfoTagged(APP_TAG, "BEGIN Renderer Initialization");
    RendererInitInfo rendererInfo = {};
    rendererInfo.appName = appInfo.appName;
    if (auto res = Renderer::init(rendererInfo); res.hasErr()) {
        return res;
    }
    logInfo("Renderer initialized SUCCESSFULLY");
    logSectionTitleInfoTagged(APP_TAG, "END Renderer Initialization");

    return {};
}

core::expected<AppError> Application::start() {
    g_appIsRunning = true;
    while (Application::isRunning()) {
        if (auto err = Platform::pollEvents(false); !err.isOk()) {
            return core::unexpected(err);
        }
        Renderer::drawFrame();
    }

    return {};
}

void Application::shutdown() {
    // TODO2: I probably need to handle termination signals at some point ? At least interupts like SIGINT (ctrl-C)
    Renderer::shutdown();
    logInfo("Renderer::shutdown finished successfully");

    Platform::shutdown();
    logInfo("Platform::shutdown finished successfully");

    core::destroyProgramCtx();
}

namespace {

core::expected<AppError> initCoreContext() {
    core::initProgramCtx(assertHandler);

    // Logger setup
    i32* tagIndicesToIgnore = nullptr;
    addr_size tagsToIgnoreSize = 0;
    // i32 tagIndicesToIgnore[] = {
    //     APP_TAG, // Default
    //     INPUT_EVENTS_TAG,
    //     RENDERER_TAG,
    //     // VULKAN_VALIDATION_TAG,
    //     X11_PLATFORM_TAG,
    // };
    // addr_size tagsToIgnoreSize = sizeof(tagIndicesToIgnore) / sizeof(tagIndicesToIgnore[0]);
    core::LoggerCreateInfo loggerCreateInfo = {};
    loggerCreateInfo.tagIndicesToIgnore = { tagIndicesToIgnore, tagsToIgnoreSize };
    if (!core::initLogger(loggerCreateInfo)) {
        return core::unexpected(createPltErr(FAILED_TO_INITIALIZE_CORE_LOGGER,
                                "Failed to initialize core logger"));
    }
    core::setLoggerLevel(core::LogLevel::L_TRACE);
    core::useAnsiInLogger(USE_ANSI_LOGGING);
    // Set logger tags
    core::setLoggerTag(APP_TAG, appLogTagsToCStr(APP_TAG));
    core::setLoggerTag(INPUT_EVENTS_TAG, appLogTagsToCStr(INPUT_EVENTS_TAG));
    core::setLoggerTag(RENDERER_TAG, appLogTagsToCStr(RENDERER_TAG));

    return {};
}

void registerEventHandlers() {
    Platform::registerWindowCloseCallback([]() {
        logInfoTagged(INPUT_EVENTS_TAG, "Closing Application!");
        g_appIsRunning = false;
    });
    Platform::registerWindowResizeCallback([](i32 w, i32 h) {
        logTraceTagged(INPUT_EVENTS_TAG, "EVENT: WINDOW_RESIZE (w=%d, h=%d)", w, h);
    });
    Platform::registerWindowFocusCallback([](bool focus) {
        if (focus) logInfoTagged(INPUT_EVENTS_TAG, "EVENT: WINDOW_FOCUS_GAINED");
        else       logInfoTagged(INPUT_EVENTS_TAG, "EVENT: WINDOW_FOCUS_LOST");
    });
    Platform::registerKeyCallback([](bool isPress, u32 vkcode, u32 scancode, KeyboardModifiers mods) {
        logTraceTagged(INPUT_EVENTS_TAG, "EVENT: KEY_%s (vkcode=%u, scancode=%u, mods=%s)",
                       isPress ? "PRESS" : "RELEASE", vkcode, scancode, keyModifiersToCptr(mods));
    });
    Platform::registerMouseClickCallback([](bool isPress, MouseButton button, i32 x, i32 y, KeyboardModifiers mods) {
        logTraceTagged(INPUT_EVENTS_TAG, "EVENT: MOUSE_%s (button=%d, x=%d, y=%d, mods=%s)",
                       isPress ? "PRESS" : "RELEASE", button, x, y, keyModifiersToCptr(mods));
    });
    Platform::registerMouseMoveCallback([](i32 x, i32 y) {
        // NOTE: Very noisy.
        logTraceTagged(INPUT_EVENTS_TAG, "EVENT: MOUSE_MOVE (x=%d, y=%d)", x, y);
    });
    Platform::registerMouseScrollCallback([](MouseScrollDirection direction, i32 x, i32 y) {
        logTraceTagged(INPUT_EVENTS_TAG, "EVENT: MOUSE_SCROLL (direction=%d, x=%d, y=%d)", direction, x, y);
    });
    Platform::registerMouseEnterOrLeaveCallback([](bool enter) {
        if (enter) logTraceTagged(INPUT_EVENTS_TAG, "EVENT: MOUSE_ENTER");
        else       logTraceTagged(INPUT_EVENTS_TAG, "EVENT: MOUSE_LEAVE");
    });

    logInfo("Registered event handlers SUCCESSFULLY");
}

void assertHandler(const char* failedExpr, const char* file, i32 line, const char* funcName, const char* errMsg) {
    // Using iostream here since assertions can happen inside core as well.

    // Get a stack trace of at max 200 stack frames, skipping the first 2. The first stack frame is this assert handler
    // frame and the second is the function itself, for which we already have information.
    constexpr u32 stackFramesToSkip = 2;
    constexpr addr_size stackTraceBufferSize = core::CORE_KILOBYTE * 8;
    char trace[stackTraceBufferSize] = {};
    addr_size traceLen = 0;
    bool ok = core::stacktrace(trace, stackTraceBufferSize, traceLen, 200, stackFramesToSkip);

    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RED_START() << ANSI_BOLD_START();
    std::cout << "[ASSERTION]:\n  [EXPR]: " << failedExpr
              << "\n  [FUNC]: " << funcName
              << "\n  [FILE]: " << file << ":" << line
              << "\n  [MSG]: " << (errMsg ? errMsg : "");
    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RESET();

    std::cout << '\n';

    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_BOLD_START();
    std::cout << "[TRACE]:\n" << trace;
    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RESET() << std::endl;

    if (!ok) {
        if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RED_START() << ANSI_BOLD_START();
        std::cout << "Failed to take full stacktrace. Consider resizing the stacktrace buffer size!";
        if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RESET();
        std::cout << std::endl;
    }

    // The only place in the code where an exception is used. Debuggers handle this in a relatively convinient way.
    throw std::runtime_error("Assertion failed!");
};

} // namespace
