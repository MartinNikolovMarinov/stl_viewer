#pragma once

#include <fwd_internal.h>
#include <platform/platform.h>

namespace stlv {

struct ApplicationState {
    bool isInitialized;

    PlatformState pltState;

    i32 startWindowWidth;
    i32 startWindowHeight;
    const char* windowTitle;
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

STLV_EXPORT bool initAppEngine(i32 argc, char** argv);
STLV_EXPORT void shutdownAppEngine();

STLV_EXPORT bool preMainLoop();
STLV_EXPORT bool updateAppState(i32& retCode);

} // namespace stlv
