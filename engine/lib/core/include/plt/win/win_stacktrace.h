#pragma once

#include <plt/core_stacktrace.h>

#include <core_utils.h>
#include <core_cptr.h>
#include <core_cptr_conv.h>

#if defined(CORE_DEBUG) && CORE_DEBUG == 1

#include <windows.h>
#include <DbgHelp.h>

#endif

namespace core {

#if defined(CORE_DEBUG) && CORE_DEBUG == 1

template <typename TAlloc>
bool stacktrace(char* buf, addr_size bufMax, addr_size& bufWritten,
                i32 nStackFrames, i32 skipFrames) {

    auto writeToBuf = [&](const char* s) -> bool {
        auto slen = core::cptrLen(s);
        if (bufWritten + slen >= bufMax) {
            return false;
        }
        bufWritten += slen;
        buf = core::cptrCopy(buf, s, slen);
        return true;
    };
    auto writeIntToBuf = [&](auto n) -> bool {
        char buf[50] = {};
        core::intToCptr(n, buf);
        return writeToBuf(buf);
    };

    // TODO: Print current thread name.

    bufWritten = 0;

    HANDLE process = GetCurrentProcess();
    bool ret = SymInitialize(process, NULL, TRUE);
    if (!ret) {
        writeToBuf("  <sym initialize failed>\n");
        return false;
    }
    defer { SymCleanup(process); };

    void** stack = reinterpret_cast<void**>(TAlloc::alloc((nStackFrames + skipFrames) * sizeof(void*)));
    defer { TAlloc::free(stack); };

    USHORT framesCount = CaptureStackBackTrace(skipFrames, nStackFrames, stack, NULL);
    if (framesCount == 0) {
        writeToBuf("  <empty, possibly corrupt>\n");
        return false;
    }

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)TAlloc::calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    defer { TAlloc::free(symbol); };

    for (int i = 0; i < framesCount; ++i) {
        if (!SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), 0, symbol)) {
            if (!writeToBuf(" <sym from addr failed>\n")) return false;
            continue;
        }

        const char* funcName = symbol->Name;

        DWORD64 displacement = 0;
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        bool ok = SymGetLineFromAddr64(process, symbol->Address, reinterpret_cast<PDWORD>(&displacement), &line);
        if (ok) {
            const char* fileName = line.FileName;
            u64 lineNumber = line.LineNumber;

            if (!writeToBuf("  "))            return false;
            if (!writeToBuf(fileName))        return false;
            if (!writeToBuf(":"))             return false;
            if (!writeIntToBuf(lineNumber))   return false;
            if (!writeToBuf("  "))            return false;
            if (!writeToBuf(funcName))        return false;
            if (!writeToBuf("\n"))            return false;
        }
        else {
            if (!writeToBuf("  "))                        return false;
            if (!writeToBuf(funcName))                    return false;
            if (!writeToBuf(" <failed to parse line>\n")) return false;
        }
    }

    return true;
}

#else

template <typename TAlloc>
bool stacktrace(char*, addr_size, addr_size&, i32, i32) { return false; }

#endif

} // namespace core
