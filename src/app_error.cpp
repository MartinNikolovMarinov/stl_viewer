#include <app_error.h>

AppError AppError::createPltErr(PlatformError::Type t, const char* msg) {
    AppError res;
    res.type = AppError::Type::PLATFORM_ERROR;
    res.pltErr.errMsg = msg;
    res.pltErr.type = t;
    return res;
}

AppError AppError::createRendErr(RendererError e) {
    AppError res;
    res.rendErr = e;
    res.type = AppError::Type::RENDERER_ERROR;
    return res;
}

bool AppError::isOk() {
    return this->type == AppError::Type::OK;
}

namespace {

constexpr const char* pltErrorToCStr(const PlatformError& e) {
    return e.errMsg ? e.errMsg : "unknown";
}

constexpr const char* rendErrorToCStr(RendererError e) {
    switch (e) {
        case RendererError::FAILED_TO_CREATE_VULKAN_INSTANCE:
            return "Failed to create Vulkan instance";
        case RendererError::FAILED_TO_GET_VULKAN_VERSION:
            return "Failed to get Vulkan version";
        case RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES:
            return "Failed to enumerate Vulkan Instance extension properties";
        case RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES:
            return "Failed to enumerate Vulkan Instance layer properties";
    }
    return "unknown";
}

} // namespace

const char* AppError::toCStr() {
    switch (this->type) {
        case Type::OK: return "ok";
        case Type::PLATFORM_ERROR: return pltErrorToCStr(this->pltErr);
        case Type::RENDERER_ERROR: return rendErrorToCStr(this->rendErr);
    }
    return "unknown";
}
