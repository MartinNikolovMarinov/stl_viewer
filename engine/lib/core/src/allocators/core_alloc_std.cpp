#include <core_alloc.h>

#include <cstdlib>
#include <atomic>

namespace core {

namespace {

OOMCallback g_oomCb = nullptr;
std::atomic<addr_size> g_totalAllocatedMem = 0;

} // namespace

void* StdAllocator::alloc(addr_size size) noexcept {
    void* ret = std::malloc(size);
    if (ret == nullptr && g_oomCb != nullptr) g_oomCb(nullptr);
    else g_totalAllocatedMem.fetch_add(size);
    return ret;
}

void* StdAllocator::calloc(addr_size count, addr_size size) noexcept {
    void* ret = std::calloc(count, size);
    if (ret == nullptr && g_oomCb != nullptr) g_oomCb(nullptr);
    else g_totalAllocatedMem.fetch_add(count * size);
    return ret;
}

void StdAllocator::free(void* ptr) noexcept {
    std::free(ptr);
}

void StdAllocator::clear() noexcept {}

addr_size StdAllocator::usedMem() noexcept {
    return 0;
}

addr_size StdAllocator::totalAllocatedMem() noexcept {
    return g_totalAllocatedMem.load();
}

bool StdAllocator::isThredSafe() noexcept {
    // Not exactly true because technically setting the global oom handler is not thread safe, but it should be set
    // exactly once in any sane program.
    return true;
}

void StdAllocator::init(OOMCallback cb) noexcept {
    g_oomCb = cb ? cb : DEFAULT_OOM_CALLBACK;
}

} // namespace core
