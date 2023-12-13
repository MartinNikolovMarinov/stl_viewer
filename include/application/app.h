#pragma once

#include <fwd_decl.h>
#include <stlv_utils.h>
#include <application/logger.h>

namespace stlv {

enum struct AppErrCode : i32 {
    SUCCESS,
    SUBMODULE_INIT_FAILURE,

    SENTINEL
};
STLV_DEFINE_FLAG_TYPE(AppErrCode);

bool isOk(AppErrCode err)  { return err == AppErrCode::SUCCESS; }
bool isErr(AppErrCode err) { return err != AppErrCode::SUCCESS; }

struct AppCreateInfo {
    i32 startPosX;
    i32 startPosY;
    i32 startWidth;
    i32 startHeight;
    const char* title;
};

struct AppInstance {
    AppCreateInfo createInfo;
    bool isRunning;
};

stlv::AppErrCode createApp(AppCreateInfo&& createInfo, AppInstance& inst) {
    inst = AppInstance{};
    inst.createInfo = core::move(createInfo);
    inst.isRunning = true;

    // Initialize subsystems
    if (!initLogging()) {
        return AppErrCode::SUBMODULE_INIT_FAILURE;
    }

    return AppErrCode::SUCCESS;
}

stlv::AppErrCode runApp(AppInstance& inst) {
    return AppErrCode::SUCCESS;
}

} // namespace stlv
