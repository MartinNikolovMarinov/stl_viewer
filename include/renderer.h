#pragma once

#include <basic.h>
#include <app_error.h>

struct Renderer {
    [[nodiscard]] static core::expected<AppError> init();
};
