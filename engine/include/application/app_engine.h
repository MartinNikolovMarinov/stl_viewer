#pragma once

#include <fwd_internal.h>

#include <application/timer.h>
#include <application/input.h>
#include <platform/platform.h>

namespace stlv {

struct AppCreateInfo {
    u32 startWindowWidth;
    u32 startWindowHeight;
    const char* windowTitle;

    bool capFrameRate;
    u32 targetFramesPerSecond;

    bool isValid();
};

struct CurrentFrameMetrics {
    Timer runningTime;
    u64 frameCount;
    f64 frameTime;
    f64 fps;
};

#define STLV_MEMORY_METRICS_ENTRY(aname) \
    u64 memoryAllocated_##aname;         \
    u64 memoryInUse_##aname

#define STLV_MEMORY_METRICS_SET(vname,aname)                                                            \
    vname.memoryAllocated_##aname = getMemoryStats(AllocationType::aname)->memoryAllocatedTotal.load(); \
    vname.memoryInUse_##aname = getMemoryStats(AllocationType::aname)->memoryInUse.load();              \

struct MemoryMetrics {
    STLV_MEMORY_METRICS_ENTRY(UNTAGGED);
    STLV_MEMORY_METRICS_ENTRY(PLATFORM);
    STLV_MEMORY_METRICS_ENTRY(RENDERER_BACKEND);
};

struct ApplicationState {
    AppCreateInfo createInfo; // Initialized in the app creation step.
    bool isInitialized; // Set by the app engine once the main loop starts.

    PlatformState pltState;

    CurrentFrameMetrics frameMetrics;
    MemoryMetrics memoryMetrics;

    Keyboard keyboard;
    Mouse mouse;
    u32 windowWidth;
    u32 windowHeight;
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
STLV_EXPORT void stopApp();

STLV_EXPORT bool initAppEngine(i32 argc, char** argv);
STLV_EXPORT void shutdownAppEngine();

STLV_EXPORT bool preMainLoop();
STLV_EXPORT bool updateAppState(i32& retCode);

} // namespace stlv
