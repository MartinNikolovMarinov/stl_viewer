#include <application/app.h>
#include <platform/platform.h>

namespace stlv {

bool isOk(AppErrCode err)  { return err == AppErrCode::SUCCESS; }
bool isErr(AppErrCode err) { return err != AppErrCode::SUCCESS; }

stlv::AppErrCode createApp(AppCreateInfo&& createInfo, AppInstance& inst) {
    inst = AppInstance{};
    inst.createInfo = core::move(createInfo);

    // Initialize submodules:
    if (!initLogging(LogLevel::DEBUG)) {
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

} // namespace stlv
