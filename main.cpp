#include <app.h>

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

i32 main() {
    core::initProgramCtx(assertHandler, nullptr);
    if (!core::initLogger(core::LoggerCreateInfo())) {
        return -1;
    }
    core::setLoggerLevel(core::LogLevel::L_TRACE);

    constexpr const char* windowTitle = "Vulkan Metal Surface Example";
    constexpr i32 windowWidth = 800, windowHeight = 600;

    if (auto err = Platform::init(windowTitle, windowWidth, windowHeight); !err.isOk()) {
        logFatal(err.toCStr());
        return -1;
    }

    using EvType = PlatformEvent::Type;

    PlatformEvent ev;
    bool stop = false;
    while (!stop) {
        if (auto err = Platform::pollEvent(ev, true); !err.isOk()) {
            logErr("Failed to poll for event, reason: %s", err.toCStr());
            return -1;
        }

        ev.logTraceEv();

        if (ev.type == EvType::WINDOW_CLOSE) {
            stop = true;
        }
    }

    return 0;
}
