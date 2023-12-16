#include <application/app.h>
#include <application/events.h>
#include <platform/platform.h>

namespace stlv {

bool isOk(AppErrCode err)  { return err == AppErrCode::SUCCESS; }
bool isErr(AppErrCode err) { return err != AppErrCode::SUCCESS; }

namespace {

auto onAppQuit = [](Event, void* context) {
    logInfo("Received quit event.");
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    inst.isRunning = false;
    return true;
};

auto onKeyDown = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received key down event. Key: %d, Scancode: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onKeyUp = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received key up event. Key: %d, Scancode: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onMouseDown = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received mouse down event. Button: %d, Action: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onMouseUp = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received mouse up event. Button: %d, Action: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onMouseScroll = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received mouse scroll event. X: %f, Y: %f", ev.data._f64[0], ev.data._f64[1]);
    return true;
};

auto onMouseMove = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received mouse move event. X: %f, Y: %f", ev.data._f64[0], ev.data._f64[1]);
    return true;
};

auto onWindowResize = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received resize event. Width: %d, Height: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onWindowMove = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;
    logTrace("Received move event. X: %d, Y: %d", ev.data._i32[0], ev.data._i32[1]);
    return true;
};

auto onWindowFocus = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;

    if (ev.data._i32[0] == 0) {
        logTrace("Received focus lost event.");
    }
    else {
        logTrace("Received focus gained event.");
    }

    return true;
};

auto onWindowHidden = [](Event ev, void* context) {
    AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
    if (!inst.isRunning) return false;

    if (ev.data._i32[0] == 0) {
        logTrace("Received window hidden event.");
        inst.isSuspended = true;
    }
    else {
        logTrace("Received window visible event.");
        inst.isSuspended = false;
    }

    return true;
};

bool registerGlobalEventHandlers(AppInstance& inst) {
    // These will, probably, never be unregistered.

    if (!eventRegister(EventCode::APP_QUIT, &inst, onAppQuit)) {
        logFatal("Failed to register APP_QUIT event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_KEY_DOWN, &inst, onKeyDown)) {
        logFatal("Failed to register APP_CODE_KEY_DOWN event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_KEY_UP, &inst, onKeyUp)) {
        logFatal("Failed to register APP_CODE_KEY_UP event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_DOWN, &inst, onMouseDown)) {
        logFatal("Failed to register APP_MOUSE_DOWN event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_UP, &inst, onMouseUp)) {
        logFatal("Failed to register APP_MOUSE_UP event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_SCROLL, &inst, onMouseScroll)) {
        logFatal("Failed to register APP_MOUSE_SCROLL event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_MOUSE_MOVE, &inst, onMouseMove)) {
        logFatal("Failed to register APP_MOUSE_MOVE event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_RESIZE, &inst, onWindowResize)) {
        logFatal("Failed to register APP_RESIZE event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_MOVE, &inst, onWindowMove)) {
        logFatal("Failed to register APP_MOVE event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_FOCUS, &inst, onWindowFocus)) {
        logFatal("Failed to register APP_FOCUS event handler.");
        return false;
    }
    if (!eventRegister(EventCode::APP_WINDOW_HIDDEN, &inst, onWindowHidden)) {
        logFatal("Failed to register APP_HIDDEN event handler.");
        return false;
    }

    return true;
}

} // namespace


stlv::AppErrCode createApp(AppCreateInfo&& createInfo, AppInstance& inst) {
    inst = AppInstance{};
    inst.createInfo = core::move(createInfo);

    core::threadingSetName("AppMain");

    // Initialize submodules

    if (!initLoggingSystem(LogLevel::L_TRACE)) {
        // Can't even log this.
        return AppErrCode::SUBMODULE_INIT_FAILURE;
    }
    logInfo("Logger initialized successfully.");

    if (!initPlatform(inst.platformState,
                      inst.createInfo.title,
                      inst.createInfo.startPosX, inst.createInfo.startPosY,
                      inst.createInfo.startWidth, inst.createInfo.startHeight)) {
        logFatal("Failed to initialize platform.");
        return AppErrCode::SUBMODULE_INIT_FAILURE;
    }

    if (!initEventSystem()) {
        logFatal("Failed to initialize event system.");
        return AppErrCode::SUBMODULE_INIT_FAILURE;
    }

    if (!registerGlobalEventHandlers(inst)) {
        return AppErrCode::SUBMODULE_INIT_FAILURE;
    }

    logInfo("Application created successfully.");
    return AppErrCode::SUCCESS;
}

stlv::AppErrCode runApp(AppInstance& inst) {
    logInfo("Running application.");

    inst.isRunning = true;
    inst.isSuspended = false;

    while (inst.isRunning) {
        f64 pollTimeout = !inst.isSuspended ? -1.0 : 2.0;
        if (!pollOsEvents(inst.platformState, pollTimeout)) {
            inst.isRunning = false;
        }

        if (!inst.isSuspended) {
            logDebug("Application render.");
        }
        else {
            logDebug("Application is suspended.");
        }
    }

    return AppErrCode::SUCCESS;
}

void destroyApp(AppInstance& inst) {
    logInfo("Destroying application.");

    destroyPlatform(inst.platformState);
    logInfo("Platform destroyed.");

    destroyLoggingSystem();
    logInfo("Logger destroyed.");

    destroyEventSystem();
    logInfo("Event system destroyed.");
}

} // namespace stlv
