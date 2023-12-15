#pragma once

#include <core_system_checks.h>

#if defined(COMPILER_MSVC) && COMPILER_MSVC == 1
    #define PRAGMA_WARNING_PUSH __pragma(warning(push))
    #define PRAGMA_WARNING_POP __pragma(warning(pop))
    #define DISABLE_MSVC_WARNING(w) __pragma(warning(disable : w))
    #define PRAGMA_COMPILER_MESSAGE(x) __pragma(message(#x))
#endif

#if defined(COMPILER_GCC) && COMPILER_GCC == 1
    #define PRAGMA_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define PRAGMA_WARNING_POP _Pragma("GCC diagnostic pop")
    #define _QUOTED_PRAGMA(x) _Pragma (#x)
    #define DISABLE_GCC_AND_CLANG_WARNING(w) _QUOTED_PRAGMA(GCC diagnostic ignored #w)
    #define DISABLE_GCC_WARNING(w) _QUOTED_PRAGMA(GCC diagnostic ignored #w)
    #define PRAGMA_COMPILER_MESSAGE(x) _QUOTED_PRAGMA(message #x)
#endif

#if defined(COMPILER_CLANG) && COMPILER_CLANG == 1
    #define PRAGMA_WARNING_PUSH _Pragma("clang diagnostic push")
    #define PRAGMA_WARNING_POP _Pragma("clang diagnostic pop")
    #define _QUOTED_PRAGMA(x) _Pragma (#x)
    #define DISABLE_GCC_AND_CLANG_WARNING(w) _QUOTED_PRAGMA(clang diagnostic ignored #w)
    #define DISABLE_CLANG_WARNING(w) _QUOTED_PRAGMA(clang diagnostic ignored #w)
    #define PRAGMA_COMPILER_MESSAGE(x) _QUOTED_PRAGMA(message #x)
#endif

#ifndef PRAGMA_WARNING_PUSH
    #define PRAGMA_WARNING_PUSH
#endif
#ifndef PRAGMA_WARNING_POP
    #define PRAGMA_WARNING_POP
#endif
#ifndef DISABLE_MSVC_WARNING
    #define DISABLE_MSVC_WARNING(...)
#endif
#ifndef DISABLE_GCC_AND_CLANG_WARNING
    #define DISABLE_GCC_AND_CLANG_WARNING(...)
#endif
#ifndef DISABLE_GCC_WARNING
    #define DISABLE_GCC_WARNING(...)
#endif
#ifndef DISABLE_CLANG_WARNING
    #define DISABLE_CLANG_WARNING(...)
#endif
#ifndef PRAGMA_COMPILER_MESSAGE
    #define PRAGMA_COMPILER_MESSAGE(...)
#endif
