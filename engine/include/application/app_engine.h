#pragma once

#include <fwd_internal.h>

#include <application/timer.h>
#include <application/input.h>
#include <platform/platform.h>

namespace stlv {

struct AppCreateInfo {
    i32 startWindowWidth;
    i32 startWindowHeight;
    const char* windowTitle;

    bool isValid();
};

struct CurrentFrameMetrics {
    Timer runningTime;
    u64 frameCount;
    f64 frameTime;
    f64 fps;
};

struct ApplicationState {
    AppCreateInfo createInfo; // Initialized in the app creation step.
    bool isInitialized; // Set by the app engine once the main loop starts.

    PlatformState pltState;

    CurrentFrameMetrics metrics;
    Keyboard keyboard;
};

/**
 * @brief Returns the application state. The applications state is valid if the following conditions are met:
 *        - The application engine has been initialized and the atomic isRunning flag is set to true.
 *        - The atomic isRunning flag is set to true.
 *
 * @remark [THREAD_SAFE]
 *
 * @return ApplicationState&
*/
STLV_EXPORT ApplicationState* getAppState();
STLV_EXPORT AppCreateInfo* getAppCreateInfo(ApplicationState* appState);

STLV_EXPORT bool initAppEngine(i32 argc, char** argv);
STLV_EXPORT void shutdownAppEngine();

STLV_EXPORT bool preMainLoop();
STLV_EXPORT bool updateAppState(i32& retCode);

} // namespace stlv
