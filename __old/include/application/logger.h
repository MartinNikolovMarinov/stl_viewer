#pragma once

#include <fwd_decl.h>

namespace stlv {

enum struct LogLevel : u8 {
    L_INPUT_TRACE = 0,
    L_DEBUG,
    L_INFO,
    L_WARNING,
    L_ERROR,
    L_FATAL,

    SENTINEL
};

bool initLoggingSystem(LogLevel minLogLevel);
void destroyLoggingSystem();

void log(LogLevel level, const char* fmt, ...);

#define logInputTrace(fmt, ...) log(LogLevel::L_INPUT_TRACE, fmt, ##__VA_ARGS__)
#define logDebug(fmt, ...)      log(LogLevel::L_DEBUG, fmt, ##__VA_ARGS__)
#define logInfo(fmt, ...)       log(LogLevel::L_INFO, fmt, ##__VA_ARGS__)
#define logWarn(fmt, ...)       log(LogLevel::L_WARNING, fmt, ##__VA_ARGS__)
#define logErr(fmt, ...)        log(LogLevel::L_ERROR, fmt, ##__VA_ARGS__)
#define logFatal(fmt, ...)      log(LogLevel::L_FATAL, fmt, ##__VA_ARGS__)

} // namespace stlv
