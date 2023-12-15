#pragma once

#ifdef CORE_LIBRARY_SHARED
    /* It's a dynamic library. */
    #ifdef CORE_LIBRARY_BUILD
        /* Building the library */
        #if defined(_MSC_VER)
            #define CORE_API_EXPORT __declspec(dllexport)
        #elif defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
            #define CORE_API_EXPORT __attribute__((visibility("default")))
        #else
            #define CORE_API_EXPORT
        #endif
    #else
        /* Using the library */
        #ifdef _WIN32
            #define CORE_API_EXPORT __declspec(dllimport)
        #else
            #define CORE_API_EXPORT
        #endif
    #endif
#endif

#ifndef CORE_API_EXPORT
    /* It's a static library, no need to import/export anything */
    #define CORE_API_EXPORT
#endif
