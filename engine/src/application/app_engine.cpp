#include <application/app_engine.h>
#include <application/logger.h>
#include <application/events.h>
#include <application/clock.h>

namespace stlv {

namespace {

ApplicationState g_appState;

core::AtomicBool g_isRunning;
core::AtomicBool g_isSuspended;

} // namespace

ApplicationState* getAppState() {
    if (g_isRunning.load() == false) {
        // Accessing the app state when the application is not running is undefined behavior.
        // This should guard agains use after free.
        return nullptr;
    }

    return &g_appState;
}

namespace {

auto onAppQuit = [](Event, void*) {
    logInfo("Received quit event.");
    g_isRunning = false;
    return true;
};

auto onKeyDown = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received key down event. Key: %d, Scancode: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onKeyUp = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received key up event. Key: %d, Scancode: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onMouseDown = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received mouse down event. Button: %d, Action: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onMouseUp = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received mouse up event. Button: %d, Action: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onMouseScroll = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received mouse scroll event. X: %f, Y: %f", ev.data._f64[0], ev.data._f64[1]);
    return true;
};

auto onMouseMove = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received mouse move event. X: %f, Y: %f", ev.data._f64[0], ev.data._f64[1]);
    return true;
};

auto onWindowResize = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received resize event. Width: %d, Height: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onWindowMove = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    logTrace("Received move event. X: %d, Y: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onWindowFocus = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;

    if (ev.data._i32[0] == 0) {
        logTrace("Received focus lost event.");
    }
    else {
        logTrace("Received focus gained event.");
    }

    return true;
};

auto onWindowHidden = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;

    if (ev.data._i32[0] == 0) {
        logTrace("Received window hidden event.");
        g_isSuspended.store(true);
    }
    else {
        logTrace("Received window visible event.");
        g_isSuspended.store(false);
    }

    return true;
};

bool registerGlobalEventHandlers() {
    // These will, probably, never be unregistered.

    if (!eventRegister(EventCode::APP_QUIT, nullptr, onAppQuit)) {
        logFatal("Failed to register APP_QUIT event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_KEY_DOWN, nullptr, onKeyDown)) {
        logFatal("Failed to register APP_CODE_KEY_DOWN event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_KEY_UP, nullptr, onKeyUp)) {
        logFatal("Failed to register APP_CODE_KEY_UP event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_DOWN, nullptr, onMouseDown)) {
        logFatal("Failed to register APP_MOUSE_DOWN event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_UP, nullptr, onMouseUp)) {
        logFatal("Failed to register APP_MOUSE_UP event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_SCROLL, nullptr, onMouseScroll)) {
        logFatal("Failed to register APP_MOUSE_SCROLL event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_MOVE, nullptr, onMouseMove)) {
        logFatal("Failed to register APP_MOUSE_MOVE event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_RESIZE, nullptr, onWindowResize)) {
        logFatal("Failed to register APP_RESIZE event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_MOVE, nullptr, onWindowMove)) {
        logFatal("Failed to register APP_MOVE event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_FOCUS, nullptr, onWindowFocus)) {
        logFatal("Failed to register APP_FOCUS event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_HIDDEN, nullptr, onWindowHidden)) {
        logFatal("Failed to register APP_HIDDEN event handler.");
        return false;
    }

    return true;
}

} // namespace

bool initAppEngine(i32 argc, char** argv) {
    ApplicationState& appState = g_appState; // app state is not initialized yet so use of getAppState is not possible.
    appState = {};

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

    if (!initEventSystem()) {
        logFatal("Failed to initialize event system.");
        return false;
    }
    logInfo("Event system initialized.");

    if (!registerGlobalEventHandlers()) {
        logFatal("Failed to register global event handlers.");
        return false;
    }
    logInfo("Global event handlers registered.");

    logInfo("Application engine initialized.");
    g_isRunning.store(true);
    g_isSuspended.store(false);
    return true;
}

bool preMainLoop() {
    logInfo("Starting pre-main loop.");

    ApplicationState& appState = g_appState;

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

    logInfo("Starting timers.");
    clockClear(appState.runningTime);
    clockStart(appState.runningTime, pltGetMonotinicTime());

    logInfo("Pre-main loop complete.");
    appState.isInitialized = true;
    return true;
}

bool updateAppState(i32& retCode) {
    if (g_isRunning.load() == false) {
        retCode = 0;
        return false; // quit
    }

    f64 pollTimeout = -1;
    if (g_isSuspended.load()) {
        logTrace("Application is suspended.");
        pollTimeout = 2.0;
    }
    else {
        logTrace("Render");
    }

    ApplicationState& appState = g_appState;

    Clock frameTimer;
    clockClear(frameTimer);
    clockStart(frameTimer, pltGetMonotinicTime());
    defer {
        clockUpdate(frameTimer, pltGetMonotinicTime());
        clockUpdate(appState.runningTime, pltGetMonotinicTime());
        appState.frameCount++;

        f64 frameTime = frameTimer.delta;
        f64 runningTime = appState.runningTime.delta;
        u64 frameCount = appState.frameCount;
        f64 fps = 1 / frameTime;

        logInfo("Frame time: %f", frameTime);
        logInfo("Running time: %f", runningTime);
        logInfo("Frame count: %llu", frameCount);
        logInfo("FPS: %f", fps);
    };

    if (!pltPollEvents(appState.pltState, pollTimeout)) {
        g_isRunning.store(false);
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
