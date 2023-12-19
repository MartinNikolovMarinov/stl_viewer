#pragma once

// IMPORTANT:
// This file initializes default settings for core and std core. Anywhere the application wants to use somthing from
// core or std core, it must include this file instead of directly including them.
// This is because defining default macros can be done only here.

#undef CORE_DEFAULT_ALLOCATOR
#define CORE_DEFAULT_ALLOCATOR() core::StdAllocator

#include <core.h>

using namespace coretypes;

bool initCore(i32 argc, char** argv);

template<> addr_size core::hash(const core::StrView& key);
template<> addr_size core::hash(const i32& key);
template<> addr_size core::hash(const u32& key);

template<> bool core::eq(const core::StrView& a, const core::StrView& b);
template<> bool core::eq(const i32& a, const i32& b);
template<> bool core::eq(const u32& a, const u32& b);

namespace stlv {

// The memory subsystem is required everywhere, so it's definition is here.

enum struct AllocationType : u8 {
    UNTAGGED = 0,

    // Allocated objects with global lifetime and variable allocation size:
    PLATFORM,
    RENDERER_BACKEND,

    // Allocated objects with specific lifetime and constant allocation size:

    // None for now..

    SENTINEL
};

struct AllocationStats {
    core::AtomicU64 memoryAllocatedTotal;
    core::AtomicU64 memoryInUse;
};
AllocationStats* getMemoryStats(AllocationType type);

using da = CORE_DEFAULT_ALLOCATOR();

namespace detail {

template <AllocationType TType>
struct STLV_Allocator {
    static constexpr const char* allocatorName() noexcept {
        switch (TType) {
            case AllocationType::UNTAGGED:         return "untagged_allocator";
            case AllocationType::PLATFORM:         return "platform_allocator";
            case AllocationType::RENDERER_BACKEND: return "renderer_backend_allocator";
            default:                               return "unknown_object_allocator";
        }
    }

    NO_COPY(STLV_Allocator);

    static void* alloc(addr_size size) noexcept {
        void* p = da::alloc(size);
        if (p == nullptr) {
            Panic(false, "Out of memory!");
        }

        getMemoryStats(TType)->memoryAllocatedTotal.fetch_add(size);
        getMemoryStats(TType)->memoryInUse.fetch_add(size);

        return p;
    }

    static void* calloc(addr_size count, addr_size size) noexcept {
        void* p = da::calloc(count, size);
        if (p == nullptr) {
            Panic(false, "Out of memory!");
        }

        getMemoryStats(TType)->memoryAllocatedTotal.fetch_add(count * size);
        getMemoryStats(TType)->memoryInUse.fetch_add(count * size);

        return p;
    }

    static void free(void* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        switch (TType) {
            // TODO: When allocator types with specific lifetime are added then memoryInUse should be decreased here.

            case AllocationType::UNTAGGED:         [[fallthrough]];
            case AllocationType::PLATFORM:         [[fallthrough]];
            case AllocationType::RENDERER_BACKEND: [[fallthrough]];
            default:
                break;
        }

        da::free(ptr);
    }

    static void clear() noexcept {
        getMemoryStats(TType)->memoryAllocatedTotal.store(0);
        getMemoryStats(TType)->memoryInUse.store(0);
    }

    static addr_size usedMem() noexcept {
        return getMemoryStats(TType)->memoryInUse.load();
    }

    static void decreaseUsedMem(addr_size size) noexcept {
        getMemoryStats(TType)->memoryInUse.fetch_sub(size);
    }

    static addr_size totalAllocatedMem() noexcept {
        return getMemoryStats(TType)->memoryAllocatedTotal.load();
    }

    static bool isThredSafe() noexcept {
        return true;
    }

    template <typename T, typename ...Args>
    static T* construct(Args&&... args) noexcept {
        void* p = STLV_Allocator::alloc(sizeof(T));
        return new (p) T(core::forward<Args>(args)...);
    }
};

} // namespace detail

using UntaggedAllocator = detail::STLV_Allocator<AllocationType::UNTAGGED>;
using PlatformAllocator = detail::STLV_Allocator<AllocationType::PLATFORM>;
using RendererBackendAllocator = detail::STLV_Allocator<AllocationType::RENDERER_BACKEND>;

static_assert(core::AllocatorConcept<PlatformAllocator>, "Allocator concept not satisfied!");
static_assert(core::AllocatorConcept<RendererBackendAllocator>, "Allocator concept not satisfied!");

inline bool initMemorySystem() {
    da::init(nullptr);
    return true;
}

inline void shutdownMemorySystem() {
    da::clear();
}

} // namespace stlv
