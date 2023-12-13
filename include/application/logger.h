#pragma once

#include <fwd_decl.h>

namespace stlv {

bool initLogging();
void shutdownLogging();

enum struct LogLevel : u8 {
    INFO = 0,
    DEBUG,
    WARNING,
    ERROR,
    FATAL,

    SENTINEL
};

void log(LogLevel level, const char* fmt, ...);

#define logInfo(fmt, ...)  log(LogLevel::INFO, fmt, ##__VA_ARGS__)
#define logDebug(fmt, ...) log(LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define logWarn(fmt, ...)  log(LogLevel::WARNING, fmt, ##__VA_ARGS__)
#define logErr(fmt, ...)   log(LogLevel::ERROR, fmt, ##__VA_ARGS__)
#define logFatal(fmt, ...) log(LogLevel::FATAL, fmt, ##__VA_ARGS__)

} // namespace stlv
