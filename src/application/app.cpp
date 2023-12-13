#include <application/app.h>
#include <application/events.h>
#include <platform/platform.h>

namespace stlv {

bool isOk(AppErrCode err)  { return err == AppErrCode::SUCCESS; }
bool isErr(AppErrCode err) { return err != AppErrCode::SUCCESS; }

stlv::AppErrCode createApp(AppCreateInfo&& createInfo, AppInstance& inst) {
    inst = AppInstance{};
    inst.createInfo = core::move(createInfo);

    core::threadingSetName("AppMain");

    // Initialize submodules

    if (!initLoggingSystem(LogLevel::DEBUG)) {
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

    // Register Global Event Handlers. These will, probably, never be unregistered.
    {
        bool ok;

        ok = eventRegister(EventCode::APP_QUIT, &inst, [](EventCode, Event, void* context) {
            logInfo("Received quit event.");
            AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
            inst.stop();
            return true;
        });
        if (!ok) {
            logFatal("Failed to register APP_QUIT event handler.");
            return AppErrCode::SUBMODULE_INIT_FAILURE;
        }

        ok = eventRegister(EventCode::APP_CODE_KEY_DOWN, &inst, [](EventCode, Event ev, void* context) {
            AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
            if (!inst.isRunning) {
                return false;
            }
            logDebug("Received key down event. Key: %d, Scancode: %d", ev.data._i32[0], ev.data._i32[1]);
            return true;
        });
        if (!ok) {
            logFatal("Failed to register APP_CODE_KEY_DOWN event handler.");
            return AppErrCode::SUBMODULE_INIT_FAILURE;
        }

        ok = eventRegister(EventCode::APP_CODE_KEY_UP, &inst, [](EventCode, Event ev, void* context) {
            AppInstance& inst = *reinterpret_cast<AppInstance*>(context);
            if (!inst.isRunning) {
                return false;
            }
            logDebug("Received key up event. Key: %d, Scancode: %d", ev.data._i32[0], ev.data._i32[1]);
            return true;
        });
        if (!ok) {
            logFatal("Failed to register APP_CODE_KEY_UP event handler.");
            return AppErrCode::SUBMODULE_INIT_FAILURE;
        }
    }

    logInfo("Application created successfully.");
    return AppErrCode::SUCCESS;
}

stlv::AppErrCode runApp(AppInstance& inst) {
    logInfo("Running application.");

    inst.isRunning = true;
    inst.isSuspended = false;

    while (inst.isRunning) {
        if (!pollOsEvents(inst.platformState)) {
            inst.isRunning = false;
        }

        if (!inst.isSuspended) {
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
