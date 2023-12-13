#include <core/init_core.h>

// Hashing global functions:

template<>
addr_size core::hash(const core::StrView& key) {
    addr_size h = addr_size(core::simpleHash_32(key.data(), key.len()));
    return h;
}

template<>
addr_size core::hash(const i32& key) {
    addr_size h = addr_size(core::simpleHash_32(reinterpret_cast<const void*>(&key), sizeof(key)));
    return h;
}

template<>
addr_size core::hash(const u32& key) {
    addr_size h = addr_size(core::simpleHash_32(reinterpret_cast<const void*>(&key), sizeof(key)));
    return h;
}

template<>
bool core::eq(const core::StrView& a, const core::StrView& b) {
    return a.eq(b);
}

template<>
bool core::eq(const i32& a, const i32& b) {
    return a == b;
}

template<>
bool core::eq(const u32& a, const u32& b) {
    return a == b;
}

void initCore() {
    core::setGlobalAssertHandler([](const char* failedExpr, const char* file, i32 line, const char* errMsg) {
        constexpr u32 stackFramesToSkip = 3;
        constexpr addr_size stackTraceBufferSize = 4096;
        char trace[stackTraceBufferSize] = {};
        addr_size traceLen = 0;
        core::stacktrace(trace, stackTraceBufferSize, traceLen, 200, stackFramesToSkip);

        fprintf(stderr,
                ANSI_BOLD(ANSI_RED("[ASSERTION] [EXPR]:")) ANSI_BOLD(" %s\n")
                ANSI_BOLD(ANSI_RED("[FILE]:"))             ANSI_BOLD(" %s:%d\n")
                ANSI_BOLD(ANSI_RED("[MSG]:"))              ANSI_BOLD(" %s\n"),

                failedExpr, file, line, errMsg
        );

        fprintf(stderr, ANSI_BOLD("[TRACE]:\n%s\n"), trace);

        throw std::runtime_error("Assertion failed!");
    });

    // Initialize the memory subsystem:
    stlv::memInit();
}

namespace stlv {

using da = CORE_DEFAULT_ALLOCATOR();

bool memInit() {
    da::init(nullptr);
    return true;
}

void memDestroy() {
    da::clear();
}

void* memAlloc(addr_size size) noexcept {
    return da::alloc(size);
}

void* memCalloc(addr_size count, addr_size size) noexcept {
    return da::calloc(count, size);
}

void memFree(void* ptr) noexcept {
    da::free(ptr);
}

addr_size memUsed() noexcept {
    return da::usedMem();
}

addr_size memTotalAllocated() noexcept {
    return da::totalAllocatedMem();
}

} // namespace stlv
