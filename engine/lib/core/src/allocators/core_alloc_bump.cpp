#include <core_alloc.h>

#include <core_mem.h>
#include <math/core_math.h>

namespace core {

namespace {

OOMCallback g_oomCb       = nullptr;
void*       g_container   = nullptr;
addr_size   g_usedMem     = 0;
addr_size   g_maxUsedMem  = 0;
addr_size   g_totalAllocatedMem = 0;

} // namespace

void* BumpAllocator::alloc(addr_size size) noexcept {
    size = core::align(size);
    if (g_usedMem + size > g_maxUsedMem) {
        if (g_oomCb != nullptr) g_oomCb(g_container);
        return nullptr;
    }
    void* ptr = reinterpret_cast<void*>(reinterpret_cast<char*>(g_container) + g_usedMem);
    g_usedMem += size;
    g_totalAllocatedMem += size;
    return ptr;
}

void* BumpAllocator::calloc(addr_size count, addr_size size) noexcept {
    addr_size totalSize = count * size;
    void* ptr = alloc(totalSize);
    if (ptr == nullptr) return nullptr;
    core::memset(ptr, 0, totalSize);
    return ptr;
}

void BumpAllocator::free(void*) noexcept { /* does nothing */ }

void BumpAllocator::clear() noexcept {
    g_usedMem = 0;
}

addr_size BumpAllocator::usedMem() noexcept {
    return g_usedMem;
}

addr_size BumpAllocator::totalAllocatedMem() noexcept {
    return g_totalAllocatedMem;
}

bool BumpAllocator::isThredSafe() noexcept {
    // It is defiantly possible to make this lock free thread safe with atomics and a CAS loop.
    return false;
}

void BumpAllocator::init(OOMCallback cb, void* container, addr_size max) noexcept {
    Panic(container != nullptr, "Bump allocator container cannot be null");
    Panic(max > 0, "Bump allocator max must be greater than 0");

    g_oomCb = cb ? cb : DEFAULT_OOM_CALLBACK;
    g_container = container;
    g_usedMem = 0;
    g_maxUsedMem = max;
}

} // namespace core
