#pragma once

#include <fwd_decl.h>
#include <stlv_utils.h>
#include <application/logger.h>
#include <platform/platform.h>

namespace stlv {

enum struct AppErrCode : i32 {
    SUCCESS,
    SUBMODULE_INIT_FAILURE,

    SENTINEL
};
STLV_DEFINE_FLAG_TYPE(AppErrCode);

bool isOk(AppErrCode err);
bool isErr(AppErrCode err);

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
    bool isSuspended;
    PlatformState platformState;

    bool stop() {
        isRunning = false;
        return true;
    }

    bool suspend() {
        isSuspended = true;
        return true;
    }
};

stlv::AppErrCode createApp(AppCreateInfo&& createInfo, AppInstance& inst);
void destroyApp(AppInstance& inst);

stlv::AppErrCode runApp(AppInstance& inst);

} // namespace stlv
