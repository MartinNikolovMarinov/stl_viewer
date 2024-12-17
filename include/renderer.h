#pragma once

#include <basic.h>

struct Renderer {
    enum struct Error {
        INIT_FAILED,
        SENTINEL,
    };
    static constexpr const char* errToCStr(Error err);

    static core::expected<Error> init();
};

constexpr const char* Renderer::errToCStr(Renderer::Error err) {
    switch (err)
    {
        case Renderer::Error::INIT_FAILED: return "Initialization Failed";

        case Renderer::Error::SENTINEL: break;
    }
    return "unknown";
}
