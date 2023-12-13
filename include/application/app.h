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
};

stlv::AppErrCode createApp(AppCreateInfo&& createInfo, AppInstance& inst);

stlv::AppErrCode runApp(AppInstance& inst);

} // namespace stlv
