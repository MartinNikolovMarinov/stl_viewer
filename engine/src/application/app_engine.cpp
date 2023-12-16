#include <application/app_engine.h>
#include <application/logger.h>

namespace stlv {

namespace {

ApplicationState g_appState;

} // namespace

ApplicationState& getAppState() {
    // TODO: Probably need a thread safe version of this. When a job/task system is introduced.
    return g_appState;
}

bool initAppEngine(i32 argc, char** argv) {
    ApplicationState& appState = getAppState();

    if (!initCore(argc, argv)) {
        // no logger available
        Assert(false, "Failed to initialize core system.");
        return false;
    }

    if (!initLoggingSystem(LogLevel::L_TRACE)) {
        // no logger available
        Assert(false, "Failed to initialize logging system.");
        return false;
    }
    logInfo("Logging system initialized.");

    if (!initPlt(appState.pltState)) {
        logFatal("Failed to initialize platform.");
        return false;
    }
    logInfo("Platform initialized.");

    logInfo("Application engine initialized.");
    return true;
}

bool preMainLoop() {
    logInfo("Starting pre-main loop.");

    ApplicationState& appState = getAppState();

    PlatformStartInfo pstartInfo = {};
    pstartInfo.windowHeight = appState.startWindowHeight;
    pstartInfo.windowWidth = appState.startWindowWidth;
    pstartInfo.windowTitle = appState.windowTitle;

    logInfo("Starting platform.");
    if (!startPlt(pstartInfo, appState.pltState)) {
        logFatal("Failed to start platform.");
        return false;
    }
    logInfo("Platform started.");

    logInfo("Pre-main loop complete.");
    return true;
}

bool updateAppState(i32& retCode) {
    ApplicationState& appState = getAppState();

    if (!pltPollEvents(appState.pltState)) {
        retCode = 0;
        return false; // quit
    }

    return true;
}

void shutdownAppEngine() {
    logInfo("Shutting down application engine.");

    shutdownLoggingSystem(); // keep this last, assume no logger availabe after this
}

} // namespace stlv
