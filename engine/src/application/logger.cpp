#include <application/logger.h>

#include <stdio.h>
#include <stdarg.h>

namespace stlv {

namespace {

constexpr addr_size BUFFER_SIZE = core::KILOBYTE * 32;
thread_local static char loggingBuffer[BUFFER_SIZE];

LogLevel minimumLogLevel = LogLevel::L_INFO;
bool ignoredTagsTable[addr_size(LogTag::SENTINEL)] = {};

} // namespace

bool initLoggingSystem(LogLevel minLogLevel, LogTag* tagsToIgnore, addr_size tagsToIgnoreCount) {
    minimumLogLevel = minLogLevel;
    if (tagsToIgnore != nullptr) {
        for (addr_size i = 0; i < tagsToIgnoreCount; ++i) {
            ignoredTagsTable[addr_size(tagsToIgnore[i])] = true;
        }
    }
    return true;
}

void shutdownLoggingSystem() {
    logInfo("Shutting down logging system");
    minimumLogLevel = LogLevel::L_INFO;
}

void __log(LogTag tag, LogLevel level, LogSpecialMode mode, const char* funcName, const char* format, ...) {
    if (level < minimumLogLevel) {
        // silence
        return;
    }
    if (ignoredTagsTable[addr_size(tag)]) {
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
        case LogLevel::L_DEBUG:
            printf(ANSI_BOLD("[DEBUG]"));
            break;
        case LogLevel::L_INFO:
            printf(ANSI_BOLD(ANSI_BRIGHT_BLUE("[INFO]")));
            break;
        case LogLevel::L_WARNING:
            printf(ANSI_BOLD(ANSI_BRIGHT_YELLOW("[WARNING]")));
            break;
        case LogLevel::L_ERROR:
            printf(ANSI_BOLD(ANSI_RED("[ERROR]")));
            break;
        case LogLevel::L_FATAL:
            printf(ANSI_BOLD(ANSI_BACKGROUND_RED(ANSI_BRIGHT_WHITE("[FATAL]"))));
            break;
        case LogLevel::L_TRACE:
            printf(ANSI_BOLD(ANSI_BRIGHT_GREEN("[INPUT_TRACE]")));
            break;

        case LogLevel::SENTINEL: [[fallthrough]];
        default:
            printf("[UNKNOWN]");
            break;
    }

    // Print Tag
    switch (tag) {
        case LogTag::T_APP:
            printf(ANSI_BOLD(ANSI_BRIGHT_WHITE("[APP]")));
            break;
        case LogTag::T_RENDERER:
            printf(ANSI_BOLD(ANSI_BRIGHT_WHITE("[RENDERER]")));
            break;

        case LogTag::T_ENGINE: [[fallthrough]];
        case LogTag::SENTINEL:
            break;
    }

    // Print Message
    if (mode == LogSpecialMode::SECTION_TITLE) {
        constexpr const char* separator = ANSI_BOLD(ANSI_BRIGHT_WHITE("---------------------------------------------------------------------"));
        printf(" _fn_(%s):\n", funcName);
        printf("%s\n", separator);
        printf("%s\n", loggingBuffer);
        printf("%s\n", separator);
    }
    else {
        printf(" _fn_(%s): %s\n", funcName, loggingBuffer);
    }
}

} // namespace stlv
