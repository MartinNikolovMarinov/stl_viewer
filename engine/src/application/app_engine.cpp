#include <application/app_engine.h>
#include <application/logger.h>
#include <application/events.h>
#include <renderer/renderer_frontend.h>

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

AppCreateInfo* getAppCreateInfo(ApplicationState* appState) {
    if (!appState) return nullptr;
    return &appState->createInfo;
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
    i32 pltKeyCode = ev.data._i32[0];
    i32 scancode = ev.data._i32[1];
    keyboardUpdate(s->keyboard, pltKeyCode, scancode, true);
    return true;
};

auto onKeyUp = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    i32 pltKeyCode = ev.data._i32[0];
    i32 scancode = ev.data._i32[1];
    keyboardUpdate(s->keyboard, pltKeyCode, scancode, false);
    return true;
};

auto onMouseDown = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    mouseUpdateClick(s->mouse, ev.data._i32[0], true);
    return true;
};

auto onMouseUp = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    mouseUpdateClick(s->mouse, ev.data._i32[0], false);
    return true;
};

auto onMouseScroll = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    mouseUpdateScroll(s->mouse, ev.data._f64[0], ev.data._f64[1]);
    return true;
};

auto onMouseMove = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    mouseUpdatePosition(s->mouse, ev.data._f64[0], ev.data._f64[1]);
    return true;
};

auto onMouseEnter = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    s->mouse.insideWindow = ev.data._bool[0];
    return true;
};

auto onWindowResize = [](Event ev, void*) {
    auto s = getAppState();
    if (!s) return false;
    s->windowHeight = ev.data._i32[0];
    s->windowWidth = ev.data._i32[1];
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

    if (ev.data._bool[0]) {
        logTrace("Received focus gained event.");
    }
    else {
        logTrace("Received focus lost event.");
    }

    return true;
};

auto onWindowHidden = [](Event ev, void*) {
    if (ev.data._bool[0]) {
        logTrace("Received window visible event.");
        g_isSuspended.store(false);
    }
    else {
        logTrace("Received window hidden event.");
        g_isSuspended.store(true);
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
    if (!eventRegister(EventCode::APP_MOUSE_ENTER, nullptr, onMouseEnter)) {
        logFatal("Failed to register APP_MOUSE_ENTER event handler.");
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
    appState = {}; // Clear the entire app state.

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

bool AppCreateInfo::isValid() {
    constexpr i32 MIN_WINDOW_WIDTH = 100;
    constexpr i32 MIN_WINDOW_HEIGHT = 100;
    if (this->startWindowHeight <= MIN_WINDOW_HEIGHT || this->startWindowWidth <= MIN_WINDOW_WIDTH) return false;
    if (!this->windowTitle) return false;

    constexpr u32 MIN_FPS = 5;
    if (this->capFrameRate) {
        if (this->targetFramesPerSecond <= MIN_FPS) return false;
    }

    return true;
}

bool preMainLoop() {
    logInfo("Starting pre-main loop.");

    ApplicationState& appState = g_appState;
    AppCreateInfo& createInfo = appState.createInfo;

    if (!createInfo.isValid()) {
        logFatal("Invalid AppCreateInfo.");
        return false;
    }
    logInfo("AppCreateInfo is valid.");

    PlatformStartInfo pstartInfo = {};
    pstartInfo.windowHeight = createInfo.startWindowHeight;
    pstartInfo.windowWidth = createInfo.startWindowWidth;
    pstartInfo.windowTitle = createInfo.windowTitle;

    appState.windowWidth = pstartInfo.windowWidth;
    appState.windowHeight = pstartInfo.windowHeight;

    logInfo("Starting platform.");
    if (!startPlt(pstartInfo, appState.pltState)) {
        logFatal("Failed to start platform.");
        return false;
    }
    logInfo("Platform started.");

    if (!initRenderer(appState.pltState)) {
        logFatal("Failed to initialize renderer.");
        return false;
    }
    logInfo("Renderer initialized.");

    logInfo("Setting up metrics.");
    clockClear(appState.frameMetrics.runningTime);
    clockStart(appState.frameMetrics.runningTime, pltGetMonotinicTime());

    keyboardClear(appState.keyboard);
    mouseClear(appState.mouse);

    logInfo("Pre-main loop complete.");
    appState.isInitialized = true;
    return true;
}

bool updateAppState(i32& retCode) {

    // Act on isRunning and isSuspended flags

    if (g_isRunning.load() == false) {
        retCode = 0;
        return false; // quit
    }

    f64 pollTimeout = -1;
    if (g_isSuspended.load()) {
        logTrace("Application is suspended.");
        pollTimeout = 2.0;
    }

    ApplicationState& appState = g_appState;

    // Start tracking frame time

    Timer frameTimer;
    clockClear(frameTimer);
    clockStart(frameTimer, pltGetMonotinicTime());
    defer {

        // Update frame metrics at the end of the frame

        auto& fmetrics = appState.frameMetrics;

        clockUpdate(frameTimer, pltGetMonotinicTime());

        // Cap frame rate
        if (appState.createInfo.capFrameRate) {
            f64 targetFrameTime = 1.0 / f64(appState.createInfo.targetFramesPerSecond);
            if (frameTimer.delta < targetFrameTime) {
                f64 remainingMs = targetFrameTime - frameTimer.delta;
                core::threadingSleep(u64(remainingMs * 1000.0));
            }
        }

        clockUpdate(frameTimer, pltGetMonotinicTime());
        clockUpdate(fmetrics.runningTime, pltGetMonotinicTime());

        fmetrics.frameTime = frameTimer.delta;
        fmetrics.frameCount = fmetrics.frameCount + 1;
        fmetrics.fps = 1.0 / fmetrics.frameTime;

        // Update memory metrics at the end of the frame
        auto& mmetrics = appState.memoryMetrics;

        STLV_MEMORY_METRICS_SET(mmetrics, UNTAGGED);
        STLV_MEMORY_METRICS_SET(mmetrics, PLATFORM);
        STLV_MEMORY_METRICS_SET(mmetrics, RENDERER_BACKEND);
    };

    // Poll the platform for events

    if (!pltPollEvents(appState.pltState, pollTimeout)) {
        g_isRunning.store(false);
        retCode = 0;
        return false; // quit
    }

    // Render the frame

    RenderPacket renderPacket = {};
    renderPacket.deltaTime = frameTimer.delta;
    rendererDrawFrame(renderPacket);

    return true;
}

void shutdownAppEngine() {
    logInfo("Shutting down application engine.");

    shutdownEventSystem();
    shutdownRenderer();
    shutdownPlt(g_appState.pltState);

    shutdownLoggingSystem(); // keep this last, assume no logger availabe after this
    shutdownMemorySystem();
}

} // namespace stlv
