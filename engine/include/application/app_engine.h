#pragma once

#include <fwd_internal.h>
#include <platform/platform.h>

namespace stlv {

struct ApplicationState {
    PlatformState pltState;

    i32 startWindowWidth;
    i32 startWindowHeight;
    const char* windowTitle;
};

STLV_EXPORT ApplicationState& getAppState();

STLV_EXPORT bool initAppEngine(i32 argc, char** argv);
STLV_EXPORT void shutdownAppEngine();

STLV_EXPORT bool preMainLoop();
STLV_EXPORT bool updateAppState(i32& retCode);

} // namespace stlv
