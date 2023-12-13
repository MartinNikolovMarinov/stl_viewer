#include <fwd_decl.h>
#include <application/logger.h>

#include <stdio.h>
#include <stdarg.h>

namespace stlv {

bool initLogging() {
    return true;
}

void shutdownLogging() {}

namespace {

constexpr addr_size BUFFER_SIZE = core::KILOBYTE * 32;
thread_local static char loggingBuffer[BUFFER_SIZE];

} // namespace


void log(LogLevel level, const char* format, ...) {
    loggingBuffer[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(loggingBuffer, BUFFER_SIZE, format, args);
    va_end(args);

    // Print Level
    switch (level) {
        case LogLevel::INFO:
            printf("%s%s[INFO]%s", ANSI_BRIGHT_BLUE_START(), ANSI_BOLD_START(), ANSI_RESET());
            break;
        case LogLevel::DEBUG:
            printf(ANSI_BOLD("[DEBUG]"));
            break;
        case LogLevel::WARNING:
            printf("%s%s[WARNING]%s", ANSI_BRIGHT_YELLOW_START(), ANSI_BOLD_START(), ANSI_RESET());
            break;
        case LogLevel::ERROR:
            printf("%s%s[ERROR]%s", ANSI_RED_START(), ANSI_BOLD_START(), ANSI_RESET());
            break;
        case LogLevel::FATAL:
            printf("%s%s%s[FATAL]%s", ANSI_BRIGHT_WHITE_START(), ANSI_BACKGROUND_RED_START(), ANSI_BOLD_START(), ANSI_RESET());
            break;
        default:
            fmt::print("[UNKNOWN] {}\n", loggingBuffer);
            break;
    }

    // Print Message
    printf(" %s\n", loggingBuffer);
}

} // namespace stlv
