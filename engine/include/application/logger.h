#pragma once

#include <fwd_internal.h>

namespace stlv {

enum struct LogLevel : u8 {
    L_TRACE = 0,
    L_DEBUG,
    L_INFO,
    L_WARNING,
    L_ERROR,
    L_FATAL,

    SENTINEL
};

enum struct LogTag : u8 {
    T_ENGINE = 0,
    T_RENDERER,
    T_APP,

    SENTINEL
};

enum struct LogSpecialMode : u8 {
    NONE = 0,

    SECTION_TITLE,

    SENTINEL
};

bool initLoggingSystem(LogLevel minLogLevel, LogTag* tagsToIgnore, addr_size tagsToIgnoreCount);
void shutdownLoggingSystem();

// Event though this is exported it should not be directly used!
STLV_EXPORT void __log(LogTag tag, LogLevel level, LogSpecialMode mode, const char* funcName, const char* format, ...);

#define logTrace(format, ...) __log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_TRACE,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logDebug(format, ...) __log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_DEBUG,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logInfo(format, ...)  __log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_INFO,    stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logWarn(format, ...)  __log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_WARNING, stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logErr(format, ...)   __log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_ERROR,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logFatal(format, ...) __log(stlv::LogTag::T_ENGINE, stlv::LogLevel::L_FATAL,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)

#define logTraceTagged(tag, format, ...) __log(tag, stlv::LogLevel::L_TRACE,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logDebugTagged(tag, format, ...) __log(tag, stlv::LogLevel::L_DEBUG,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logInfoTagged(tag, format, ...)  __log(tag, stlv::LogLevel::L_INFO,    stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logWarnTagged(tag, format, ...)  __log(tag, stlv::LogLevel::L_WARNING, stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logErrTagged(tag, format, ...)   __log(tag, stlv::LogLevel::L_ERROR,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)
#define logFatalTagged(tag, format, ...) __log(tag, stlv::LogLevel::L_FATAL,   stlv::LogSpecialMode::NONE, __func__, format, ##__VA_ARGS__)

#define logSectionTitleTraceTagged(tag, format, ...) __log(tag, stlv::LogLevel::L_TRACE,   stlv::LogSpecialMode::SECTION_TITLE, __func__, format, ##__VA_ARGS__)
#define logSectionTitleDebugTagged(tag, format, ...) __log(tag, stlv::LogLevel::L_DEBUG,   stlv::LogSpecialMode::SECTION_TITLE, __func__, format, ##__VA_ARGS__)
#define logSectionTitleInfoTagged(tag, format, ...)  __log(tag, stlv::LogLevel::L_INFO,    stlv::LogSpecialMode::SECTION_TITLE, __func__, format, ##__VA_ARGS__)
#define logSectionTitleWarnTagged(tag, format, ...)  __log(tag, stlv::LogLevel::L_WARNING, stlv::LogSpecialMode::SECTION_TITLE, __func__, format, ##__VA_ARGS__)
#define logSectionTitleErrTagged(tag, format, ...)   __log(tag, stlv::LogLevel::L_ERROR,   stlv::LogSpecialMode::SECTION_TITLE, __func__, format, ##__VA_ARGS__)
#define logSectionTitleFatalTagged(tag, format, ...) __log(tag, stlv::LogLevel::L_FATAL,   stlv::LogSpecialMode::SECTION_TITLE, __func__, format, ##__VA_ARGS__)

} // namespace stlv
