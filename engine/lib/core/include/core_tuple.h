#pragma once

#include <core_types.h>
#include <core_utils.h>
#include <core_traits.h>

namespace core {

using namespace coretypes;

template <typename...> struct tuple;

template <typename T1, typename T2>
struct tuple<T1, T2> {
    static constexpr u32 len = 2;

    template <i32 TIdx>
    constexpr auto& get() {
        if constexpr (TIdx == 0) {
            return v1;
        }
        else if constexpr (TIdx == 1) {
            return v2;
        }
        else {
            static_assert(core::always_false<T1>, "Invalid get index.");
            return v1;
        }
    }

    template <i32 TIdx>
    constexpr const auto& get() const {
        if constexpr (TIdx == 0) {
            return v1;
        }
        else if constexpr (TIdx == 1) {
            return v2;
        }
        else {
            static_assert(core::always_false<T1>, "Invalid get index.");
            return v1;
        }
    }

    T1 v1;
    T2 v2;
};

template <typename T1, typename T2, typename T3>
struct tuple<T1, T2, T3> {
    static constexpr u32 len = 3;

    template <i32 TIdx>
    constexpr auto& get() {
        if constexpr (TIdx == 0) {
            return v1;
        }
        else if constexpr (TIdx == 1) {
            return v2;
        }
        else if constexpr (TIdx == 2) {
            return v3;
        }
        else {
            static_assert(core::always_false<T1>, "Invalid get index.");
            return v1;
        }
    }

    template <i32 TIdx>
    constexpr const auto& get() const {
        if constexpr (TIdx == 0) {
            return v1;
        }
        else if constexpr (TIdx == 1) {
            return v2;
        }
        else if constexpr (TIdx == 2) {
            return v3;
        }
        else {
            static_assert(core::always_false<T1>, "Invalid get index.");
            return v1;
        }
    }

    T1 v1;
    T2 v2;
    T3 v3;
};

template <typename T1, typename T2, typename T3, typename T4>
struct tuple<T1, T2, T3, T4> {
    static constexpr u32 len = 4;

    template <i32 TIdx>
    constexpr auto& get() {
        if constexpr (TIdx == 0) {
            return v1;
        }
        else if constexpr (TIdx == 1) {
            return v2;
        }
        else if constexpr (TIdx == 2) {
            return v3;
        }
        else if constexpr (TIdx == 3) {
            return v4;
        }
        else {
            static_assert(core::always_false<T1>, "Invalid get index.");
            return v1;
        }
    }

    template <i32 TIdx>
    constexpr const auto& get() const {
        if constexpr (TIdx == 0) {
            return v1;
        }
        else if constexpr (TIdx == 1) {
            return v2;
        }
        else if constexpr (TIdx == 2) {
            return v3;
        }
        else if constexpr (TIdx == 3) {
            return v4;
        }
        else {
            static_assert(core::always_false<T1>, "Invalid get index.");
            return v1;
        }
    }

    T1 v1;
    T2 v2;
    T3 v3;
    T4 v4;
};

template <typename...TArgs>
constexpr tuple<TArgs...> createTuple(TArgs&&... args) {
    constexpr i32 NArgs = sizeof...(TArgs);
    static_assert(1 < NArgs && NArgs <= 4, "tuples can only have 2, 3 or 4 elements");
    if constexpr (NArgs == 2) {
        return tuple<TArgs...>{ core::forward<TArgs>(args)... };
    }
    else if constexpr (NArgs == 3) {
        return tuple<TArgs...>{ core::forward<TArgs>(args)... };
    }
    else if constexpr (NArgs == 4) {
        return tuple<TArgs...>{ core::forward<TArgs>(args)... };
    }
    else {
        static_assert(core::always_false<TArgs...>, "Can't create tuple with that many arguments.");
    }
}

template <typename T> using pair   = tuple<T, T>;
template <typename T> using triple = tuple<T, T, T>;
template <typename T> using quad   = tuple<T, T, T, T>;

static_assert(core::is_pod_v<pair<i32>>);
static_assert(core::is_pod_v<triple<i32>>);
static_assert(core::is_pod_v<quad<i32>>);

} // namespace core
