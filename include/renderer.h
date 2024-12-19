#pragma once

#include <basic.h>
#include <app_error.h>

struct Renderer {
    static constexpr addr_size VERSION_BUFFER_SIZE = 255;
    [[nodiscard]] static core::expected<AppError> getVersion(char version[VERSION_BUFFER_SIZE]);

    [[nodiscard]] static core::expected<AppError> init();
};
