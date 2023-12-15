#include <core_alloc.h>

#include <cstdlib>

namespace core {

namespace {

struct AllocatedBlock {
    void* addr;
    AllocatedBlock* next;
    addr_size size;
};

struct AllocatedBlockList {

    AllocatedBlock* head() noexcept { return m_head; }
    AllocatedBlock* tail() noexcept { return m_tail; }
    addr_size used() noexcept { return m_used; }

    void addBlock(AllocatedBlock* block) noexcept {
        if (m_head == nullptr) {
            m_head = block;
            m_tail = block;
            block->next = nullptr;
        }
        else {
            m_tail->next = block;
            m_tail = block;
            block->next = nullptr;
        }

        m_used += block->size;
    }

    void removeBlock(void* addr) noexcept {
        if (m_head == nullptr) return;

        AllocatedBlock* block = nullptr;
        if (m_head->addr == addr) {
            block = m_head;
            m_head = m_head->next;
            if (m_head == nullptr) m_tail = nullptr;
        } else {
            AllocatedBlock* prev = m_head;
            block = m_head->next;
            while (block != nullptr) {
                if (block->addr == addr) {
                    prev->next = block->next;
                    if (block->next == nullptr) m_tail = prev;
                    break;
                }
                prev = block;
                block = block->next;
            }
        }

        if (block != nullptr) {
            m_used -= block->size;
            std::free(block);
        }
    }

    void freeBlocks() noexcept {
        AllocatedBlock* block = m_head;
        while (block != nullptr) {
            AllocatedBlock* next = block->next;
            std::free(block);
            block = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_used = 0;
    }

private:
    AllocatedBlock* m_head = nullptr;
    AllocatedBlock* m_tail = nullptr;
    addr_size m_used = 0; // in bytes
};

OOMCallback g_oomCb = nullptr;
AllocatedBlockList g_allocatedBlocks;
addr_size g_totalAllocatedMem = 0;

AllocatedBlock* trackMemory(void* addr, addr_size size) {
    auto block = static_cast<AllocatedBlock*>(std::malloc(sizeof(AllocatedBlock)));
    if (block == nullptr) {
        if (g_oomCb != nullptr) g_oomCb(nullptr);
        return nullptr;
    }

    block->addr = addr;
    block->size = size;
    g_allocatedBlocks.addBlock(block);
    g_totalAllocatedMem += size;
    return block;
}

} // namespace

void* StdStatsAllocator::alloc(addr_size size) noexcept {
    if (size == 0) return nullptr;

    void* addr = std::malloc(size);

    if (addr == nullptr) {
        if (g_oomCb != nullptr) g_oomCb(nullptr);
        return nullptr;
    }

    if (!trackMemory(addr, size)) {
        // not enough memory to keep track of this allocation.
        std::free(addr); // give up on this memory
        return nullptr;
    }

    return addr;
}

void* StdStatsAllocator::calloc(addr_size count, addr_size size) noexcept {
    if (count == 0 || size == 0) return nullptr;

    void* addr = std::calloc(count, size);

    if (addr == nullptr) {
        if (g_oomCb != nullptr) g_oomCb(nullptr);
        return nullptr;
    }

    if (!trackMemory(addr, count * size)) {
        // not enough memory to keep track of this allocation.
        std::free(addr); // give up on this memory
        return nullptr;
    }

    return addr;
}

void StdStatsAllocator::free(void* ptr) noexcept {
    if (ptr == nullptr) return;
    g_allocatedBlocks.removeBlock(ptr);
    std::free(ptr);
}

void StdStatsAllocator::clear() noexcept {
    auto block = g_allocatedBlocks.head();
    while (block != nullptr) {
        auto next = block->next;
        std::free(block->addr);
        block = next;
    }
    g_allocatedBlocks.freeBlocks();
}

addr_size StdStatsAllocator::usedMem() noexcept {
    return g_allocatedBlocks.used();
}

addr_size StdStatsAllocator::totalAllocatedMem() noexcept {
    return g_totalAllocatedMem;
}

bool StdStatsAllocator::isThredSafe() noexcept {
    // NOTE: The only feasible way to make this thread safe is to use a mutex.
    return false;
}

void StdStatsAllocator::init(OOMCallback cb) noexcept {
    g_oomCb = cb ? cb : DEFAULT_OOM_CALLBACK;
}

} // namespace core
