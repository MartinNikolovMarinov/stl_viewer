#pragma once

#include <core_API.h>
#include <core_types.h>
#include <core_alloc.h>

namespace core {

using namespace coretypes;

template <typename TAlloc = CORE_DEFAULT_ALLOCATOR()>
bool stacktrace(char* buf, addr_size bufMax, addr_size& bufWritten, i32 nStackFrames, i32 skipFrames = 1);

} // namespace core

#if defined(OS_WIN) && OS_WIN == 1
    #include <plt/win/win_stacktrace.h>
#elif defined(OS_LINUX) && OS_LINUX == 1
    #include <plt/unix/unix_stacktrace.h>
#elif defined(OS_MAC) && OS_MAC == 1
    #include <plt/unix/unix_stacktrace.h>
#endif
