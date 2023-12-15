#include <plt/core_pages.h>

#include <windows.h>

namespace core {

expected<void*, PltErrCode> allocPages(size_t count) {
    void* addr = VirtualAlloc(nullptr, count, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (addr == nullptr) {
        return core::unexpected(PltErrCode(GetLastError()));
    }

    return addr;
}

expected<PltErrCode> freePages(void* addr, size_t) {
    if (addr == nullptr) {
        return core::unexpected(PltErrCode(EINVAL));
    }

    // Size must be 0 when using MEM_RELEASE.
    BOOL err = VirtualFree(addr, 0, MEM_RELEASE);
    if (err == 0) {
        return core::unexpected(PltErrCode(GetLastError()));
    }

    return {};
}

addr_size getPageSize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    addr_size ret = addr_size(si.dwPageSize);
    return ret;
}

} // namespace core
