#pragma once

#include <fwd_decl.h>

namespace stlv {

enum struct LogLevel : u8 {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL,

    SENTINEL
};

bool initLoggingSystem(LogLevel minLogLevel);
void destroyLoggingSystem();

void log(LogLevel level, const char* fmt, ...);

#define logDebug(fmt, ...) log(LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define logInfo(fmt, ...)  log(LogLevel::INFO, fmt, ##__VA_ARGS__)
#define logWarn(fmt, ...)  log(LogLevel::WARNING, fmt, ##__VA_ARGS__)
#define logErr(fmt, ...)   log(LogLevel::ERROR, fmt, ##__VA_ARGS__)
#define logFatal(fmt, ...) log(LogLevel::FATAL, fmt, ##__VA_ARGS__)

} // namespace stlv
