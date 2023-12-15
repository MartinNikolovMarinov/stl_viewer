#pragma once

#include <core_API.h>
#include <core_traits.h>
#include <core_types.h>
#include <core_utils.h>
#include <core_compiler.h>

#include <new>

namespace core {

using namespace coretypes;

/**
 * @brief This macro should be overriden by the user to specify the default allocator. If it is not overriden, the
 *        DEFAULT template type for most data structures will fail to compile.
*/
#ifndef CORE_DEFAULT_ALLOCATOR
    #define CORE_DEFAULT_ALLOCATOR() void
#endif

using OOMCallback = void (*)(void* userData);

static constexpr OOMCallback DEFAULT_OOM_CALLBACK = [](void*) {
    Panic(false, "Out of memory!");
};

template <typename T>
concept AllocatorConcept = requires {
    { T::allocatorName() } noexcept -> core::same_as<const char*>;
    { T::alloc(core::declval<addr_size>()) } noexcept -> core::same_as<void*>;
    { T::calloc(core::declval<addr_size>(), core::declval<addr_size>()) } noexcept -> core::same_as<void*>;
    { T::free(core::declval<void*>()) } noexcept;
    { T::usedMem() } noexcept -> core::same_as<addr_size>;
    { T::totalAllocatedMem() } noexcept -> core::same_as<addr_size>;
    { T::isThredSafe() } noexcept -> core::same_as<bool>;

    // should be able to construct and integer:
    { T::template construct<i32>(core::declval<i32>()) } noexcept -> core::same_as<i32*>;
};

struct CORE_API_EXPORT StdAllocator {
    static constexpr const char* allocatorName() noexcept { return "std_allocator"; }

    NO_COPY(StdAllocator);

    static void* alloc(addr_size size) noexcept;
    static void* calloc(addr_size count, addr_size size) noexcept;
    static void free(void* ptr) noexcept;
    static void clear() noexcept; // does nothing
    static addr_size usedMem() noexcept; // does nothing
    static addr_size totalAllocatedMem() noexcept;
    static bool isThredSafe() noexcept;

    template <typename T, typename ...Args>
    static T* construct(Args&&... args) noexcept {
        void* p = StdAllocator::alloc(sizeof(T));
        return new (p) T(core::forward<Args>(args)...);
    }

    static void init(OOMCallback cb) noexcept;
};

static_assert(AllocatorConcept<StdAllocator>, "StdAllocator does not satisfy AllocatorConcept");

struct CORE_API_EXPORT StdStatsAllocator {
    static constexpr const char* allocatorName() noexcept { return "std_stats_allocator"; }

    NO_COPY(StdStatsAllocator);

    static void* alloc(addr_size size) noexcept;
    static void* calloc(addr_size count, addr_size size) noexcept;
    static void free(void* ptr) noexcept;
    static void clear() noexcept;
    static addr_size usedMem() noexcept;
    static addr_size totalAllocatedMem() noexcept;
    static bool isThredSafe() noexcept;

    template <typename T, typename ...Args>
    static T* construct(Args&&... args) noexcept {
        void* p = StdStatsAllocator::alloc(sizeof(T));
        return new (p) T(core::forward<Args>(args)...);
    }

    static void init(OOMCallback cb) noexcept;
};

static_assert(AllocatorConcept<StdStatsAllocator>, "StdStatsAllocator does not satisfy AllocatorConcept");

struct CORE_API_EXPORT BumpAllocator {
    static constexpr const char* allocatorName() noexcept { return "bump_allocator"; }

    NO_COPY(BumpAllocator);

    static void* alloc(addr_size size) noexcept;
    static void* calloc(addr_size count, addr_size size) noexcept;
    static void free(void* ptr) noexcept; // does nothing
    static void clear() noexcept;
    static addr_size usedMem() noexcept;
    static addr_size totalAllocatedMem() noexcept;
    static bool isThredSafe() noexcept;

    template <typename T, typename ...Args>
    static T* construct(Args&&... args) noexcept {
        void* p = BumpAllocator::alloc(sizeof(T));
        return new (p) T(core::forward<Args>(args)...);
    }

    static void init(OOMCallback cb, void* container, addr_size max) noexcept;
};

static_assert(AllocatorConcept<BumpAllocator>, "BumpAllocator does not satisfy AllocatorConcept");

} // namespace core
