#include <app.h>
#include <platform.h>
#include <user_input.h>

#include <iostream> // TODO2: Do I want to use c++ iostream even just for assertions it kinda sucks.

using PltEvType = PlatformEvent::Type;

using PlatformError::Type::FAILED_TO_INITIALIZE_CORE_LOGGER;

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

    // TODO: Initialize the renderer

    return {};
}

core::expected<AppError> Application::start() {
    PlatformEvent ev;
    bool stop = false;
    while (!stop) {
        if (auto err = Platform::pollEvent(ev, true); !err.isOk()) {
            return core::unexpected(err);
        }

        ev.logTraceEv();

        if (ev.type == PltEvType::WINDOW_CLOSE) {
            stop = true;
        }
    }

    return {};
}

void Application::shutdown() {
    Platform::shutdown();
}
