#pragma once

#include <fwd_internal.h>

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

enum struct LogTag : u8 {
    T_ENGINE = 0,
    T_APP,

    SENTINEL
};

bool initLoggingSystem(LogLevel minLogLevel);
void shutdownLoggingSystem();

STLV_EXPORT void log(LogTag tag, LogLevel level, const char* fmt, ...);

#define logInputTrace(fmt, ...) log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_INPUT_TRACE, fmt, ##__VA_ARGS__)
#define logDebug(fmt, ...)      log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_DEBUG, fmt, ##__VA_ARGS__)
#define logInfo(fmt, ...)       log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_INFO, fmt, ##__VA_ARGS__)
#define logWarn(fmt, ...)       log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_WARNING, fmt, ##__VA_ARGS__)
#define logErr(fmt, ...)        log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_ERROR, fmt, ##__VA_ARGS__)
#define logFatal(fmt, ...)      log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_FATAL, fmt, ##__VA_ARGS__)

#define logInputTraceTagged(tag, fmt, ...) log(tag, stlv::LogLevel::L_INPUT_TRACE, fmt, ##__VA_ARGS__)
#define logDebugTagged(tag, fmt, ...)      log(tag, stlv::LogLevel::L_DEBUG, fmt, ##__VA_ARGS__)
#define logInfoTagged(tag, fmt, ...)       log(tag, stlv::LogLevel::L_INFO, fmt, ##__VA_ARGS__)
#define logWarnTagged(tag, fmt, ...)       log(tag, stlv::LogLevel::L_WARNING, fmt, ##__VA_ARGS__)
#define logErrTagged(tag, fmt, ...)        log(tag, stlv::LogLevel::L_ERROR, fmt, ##__VA_ARGS__)
#define logFatalTagged(tag, fmt, ...)      log(tag, stlv::LogLevel::L_FATAL, fmt, ##__VA_ARGS__)

} // namespace stlv
