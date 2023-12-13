#include <fwd_decl.h>
#include <application/logger.h>

#include <stdio.h>
#include <stdarg.h>

namespace stlv {

namespace {

constexpr addr_size BUFFER_SIZE = core::KILOBYTE * 32;
thread_local static char loggingBuffer[BUFFER_SIZE];

LogLevel minimumLogLevel = LogLevel::INFO;

} // namespace

bool initLogging(LogLevel minLogLevel) {
    minimumLogLevel = minLogLevel;
    return true;
}

void shutdownLogging() {}

void log(LogLevel level, const char* format, ...) {
    if (level < minimumLogLevel) {
        // silence
        return;
    }

    loggingBuffer[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(loggingBuffer, BUFFER_SIZE, format, args);
    va_end(args);

    // Print Level
    switch (level) {
        case LogLevel::DEBUG:
            printf(ANSI_BOLD("[DEBUG]"));
            break;
        case LogLevel::INFO:
            printf(ANSI_BOLD(ANSI_BRIGHT_BLUE("[INFO]")));
            break;
        case LogLevel::WARNING:
            printf(ANSI_BOLD(ANSI_BRIGHT_YELLOW("[WARNING]")));
            break;
        case LogLevel::ERROR:
            printf(ANSI_BOLD(ANSI_RED("[ERROR]")));
            break;
        case LogLevel::FATAL:
            printf(ANSI_BOLD(ANSI_BACKGROUND_RED(ANSI_BRIGHT_WHITE("[FATAL]"))));
            break;
        default:
            printf("[UNKNOWN]");
            break;
    }

    // Print Message
    printf(" %s\n", loggingBuffer);
}

} // namespace stlv
