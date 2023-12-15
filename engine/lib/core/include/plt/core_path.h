#pragma once

#include <core_alloc.h>
#include <core_API.h>
#include <core_arr.h>
#include <core_cptr.h>
#include <core_traits.h>
#include <core_types.h>
#include <core_utf.h>

namespace core {

static constexpr char PATH_SEPARATOR = '/';

template <typename T>
concept Path =
    requires {
        requires core::is_same_v<decltype(&T::path), const char* (T::*)() const>;
        requires core::is_same_v<decltype(&T::filePart), const char* (T::*)() const>;
        requires core::is_same_v<decltype(&T::extPart), const char* (T::*)() const>;
    };

struct CORE_API_EXPORT ImmutablePath {
    const char* data;
    const addr_size len;

    ImmutablePath(const char* p, addr_size l) : data(p), len(l) {}
    ImmutablePath(const char* p) : data(p), len(core::cptrLen(p)) {}

    const char* filePart() const {
        const char* p = data + len - 1;
        while (p != data && *p != PATH_SEPARATOR) {
            --p;
        }

        return p;
    }

    const char* extPart() const {
        const char* p = filePart();
        while (p != data && *p != '.') {
            --p;
        }

        return p;
    }

    const char* path() const { return data; }
};

static_assert(Path<ImmutablePath>, "ImmutablePath does not satisfy Path concept");

} // namespace core
