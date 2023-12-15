#pragma once

#include <core_API.h>
#include <core_types.h>
#include <core_traits.h>

namespace core {

using namespace coretypes;

// General Macro Magic for matching different macros for different number of arguments:
#define __C_RSEQ_N__ 10,9,8,7,6,5,4,3,2,1,0
#define __C_ARG_N__(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, N, ...) N
#define __EXPAND_ARGS(args) __C_ARG_N__ args
#define __COUNT_ARGS_MAX10(...) __EXPAND_ARGS((__VA_ARGS__, __C_RSEQ_N__))
#define __OVERLOAD_MACRO2(name, count) name##count
#define __OVERLOAD_MACRO1(name, count) __OVERLOAD_MACRO2(name, count)
#define __OVERLOAD_MACRO(name, count) __OVERLOAD_MACRO1(name, count)

#define C_VFUNC(func, ...) __OVERLOAD_MACRO(func, __COUNT_ARGS_MAX10(__VA_ARGS__)) (__VA_ARGS__)

// Customizeble global assert handler:
using globalAssertHandlerPtr = void (*)(const char* failedExpr, const char* file, i32 line, const char* errMsg);
CORE_API_EXPORT void setGlobalAssertHandler(globalAssertHandlerPtr handler);
CORE_API_EXPORT globalAssertHandlerPtr getGlobalAssertHandler();

#if defined(CORE_ASSERT_ENABLED) && CORE_ASSERT_ENABLED
    #ifndef Assert
        // This macro will dereference null to force a crash,
        // unless the global assert handler is set and returns false,
        // or exists the program in some other way.
        #define Assert(...) C_VFUNC(Assert, __VA_ARGS__)
        #define Assert1(expr) Assert2(expr, "")
        #define Assert2(expr, msg)                                                  \
            if (!(expr)) {                                                          \
                if (core::getGlobalAssertHandler()) {                               \
                    core::getGlobalAssertHandler()(#expr, __FILE__, __LINE__, msg); \
                } else {                                                            \
                    i32* __forceCrash = nullptr;                                    \
                    [[maybe_unused]] i32 __ignored = *__forceCrash;                 \
                }                                                                   \
            }
    #endif
#else
    #define Assert(...)
#endif

#ifndef Panic
    #if defined(CORE_ASSERT_ENABLED) && CORE_ASSERT_ENABLED
        #define Panic(...) Assert(__VA_ARGS__)
    #else
        #define Panic(...) *reinterpret_cast<volatile coretypes::i32 *>(0) = 0;
    #endif
#endif

// Zero cost defer:
#ifndef defer
    struct CORE_API_EXPORT deferDummy {};
    template <typename F> struct deferrer { F f; ~deferrer() { f(); } };
    template <typename F> deferrer<F> operator*(core::deferDummy, F f) { return {f}; }
    #define DEFER_(LINE) zz_defer##LINE
    #define DEFER(LINE) DEFER_(LINE)
    #define defer auto DEFER(__LINE__) = core::deferDummy{} *[&]()
#endif

// No copy macro:

#define NO_COPY(T) \
    T(const T&) = delete; \
    T& operator=(const T&) = delete

// No move macro:

#define NO_MOVE(T) \
    T(T&&) = delete; \
    T& operator=(T&&) = delete

// Add this macro to functions that are not supposed to be mangled:

#define NO_MANGLE extern "C"

// Move and forward implementations copied from the standard library:

template<typename T>
constexpr remove_ref_t<T> && move(T && arg) {
    return static_cast<remove_ref_t<T> &&>(arg);
}

template<typename T>
constexpr T&& forward(remove_ref_t<T> & arg) {
    return static_cast<T&&>(arg);
}

template<typename T>
constexpr T&& forward(remove_ref_t<T> && arg) {
    static_assert(!is_lvalue<T>::value, "invalid rvalue to lvalue conversion");
    return static_cast<T&&>(arg);
}

} // namespace core
