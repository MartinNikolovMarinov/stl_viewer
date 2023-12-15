#include <plt/core_pages.h>

#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

namespace core {

expected<void*, PltErrCode> allocPages(addr_size count) {
     // flags - memory is private copy-on-write and is not backed by a file, i.e. anonymous
    i32 flags = ( MAP_PRIVATE | MAP_ANONYMOUS );
    // port - memory is mapped for reading and for writing.
    i32 prot = ( PROT_READ | PROT_WRITE );

    void* addr = mmap(nullptr, count, prot, flags, 0, 0);
    if (addr == MAP_FAILED || addr == nullptr) {
        return core::unexpected(PltErrCode(errno));
    }

    return addr;
}

expected<PltErrCode> freePages(void* addr, addr_size count) {
    if (addr == nullptr) {
        return core::unexpected(PltErrCode(EINVAL));
    }

    i32 err = munmap(addr, count);
    if (err != 0) {
        return core::unexpected(PltErrCode(errno));
    }

    return {};
}

addr_size getPageSize() {
    addr_size ret = addr_size(sysconf(_SC_PAGESIZE));
    return ret;
}

} // namespace core
