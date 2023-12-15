#pragma once

#include <plt/core_stacktrace.h>

#include <core_cptr_conv.h>
#include <core_cptr.h>
#include <core_mem.h>
#include <core_utils.h>

#include <execinfo.h>
#include <cxxabi.h>

namespace core {

template <typename TAlloc>
bool stacktrace(char* buf, addr_size bufMax, addr_size& bufWritten, int nStackFrames, int skipFrames) {
    auto writeToBuf = [&](const char* s) -> bool {
        auto slen = core::cptrLen(s);
        if (bufWritten + slen >= bufMax) {
            return false;
        }
        bufWritten += slen;
        buf = core::cptrCopy(buf, s, slen);
        return true;
    };

    // TODO: Print current thread name.

    bufWritten = 0;

    // Capture the backtrace
    void** callstack = reinterpret_cast<void**>(TAlloc::alloc(addr_size(nStackFrames + skipFrames) * sizeof(void*)));
    i32 framesCount = backtrace(callstack, nStackFrames + skipFrames - 1);
    if (framesCount == 0) {
        writeToBuf("  <empty, possibly corrupt>\n");
        return false;
    }
    defer { TAlloc::free(callstack); };

    // Get the symbols
    char** symbols = backtrace_symbols(callstack, framesCount);
    if (symbols == nullptr) {
        writeToBuf("  <failed to backtrace symbols>\n");
        return false;
    }
    defer { TAlloc::free(symbols); };

    for (i32 i = skipFrames; i < framesCount; i++) {
        char* symbol = symbols[i];

        char* beginName = nullptr;
        char* beginOffset = nullptr;
        char* endOffset = nullptr;

        // Find the parentheses and +address offset surrounding the mangled name
        for (char* p = symbols[i]; *p; ++p) {
            if (*p == '(') {
                beginName = p;
            }
            else if (*p == '+') {
                beginOffset = p;
            }
            else if (*p == ')' && beginOffset) {
                endOffset = p;
                break;
            }
        }

        bool successfullyParsedLine = beginName && beginOffset && endOffset && beginName < beginOffset;

        if (successfullyParsedLine) {
            *beginName++ = core::term_char;
            *beginOffset++ = core::term_char;
            *endOffset = core::term_char;

            i32 status = 0;
            char* demangled = abi::__cxa_demangle(beginName, nullptr, nullptr, &status);
            defer { if (demangled) TAlloc::free(demangled); };

            bool demangleSuccess = (status == 0);

            if (demangleSuccess) {
                if (!writeToBuf("  "))        return false;
                if (!writeToBuf(symbol))      return false;
                if (!writeToBuf(" : "))       return false;
                if (!writeToBuf(demangled))   return false;
                if (!writeToBuf(" +"))        return false;
                if (!writeToBuf(beginOffset)) return false;
                if (!writeToBuf("\n"))        return false;
            }
            else {
                // Demangling failed. Output mangled function:
                if (!writeToBuf("  "))                             return false;
                if (!writeToBuf(symbol))                           return false;
                if (!writeToBuf(" : "))                            return false;
                if (!writeToBuf(beginName))                        return false;
                if (!writeToBuf(" +"))                             return false;
                if (!writeToBuf(beginOffset))                      return false;
                if (!writeToBuf(" <demangling failed: status = ")) return false;
                {
                    char strStatus[20] = {};
                    core::intToCptr(status, strStatus);
                    if (!writeToBuf(strStatus)) return false;
                }
                if (!writeToBuf(">\n")) return false;
            }
        }
        else {
            if (!writeToBuf("  "))                        return false;
            if (!writeToBuf(symbol))                      return false;
            if (!writeToBuf(" <failed to parse line>\n")) return false;
            if (!writeToBuf("\n"))                        return false;
        }
    }

    return true;
}

} // namespace core
