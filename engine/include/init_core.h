#pragma once

// IMPORTANT:
// This file initializes default settings for core and std core. Anywhere the application wants to use somthing from
// core or std core, it must include this file instead of directly including them.
// This is because defining default macros can be done only here.

#undef CORE_DEFAULT_ALLOCATOR
#define CORE_DEFAULT_ALLOCATOR() core::StdAllocator

#include <core.h>

using namespace coretypes;

void initCore();

using Sb = core::StrBuilder<>;

template<> addr_size core::hash(const core::StrView& key);
template<> addr_size core::hash(const i32& key);
template<> addr_size core::hash(const u32& key);

template<> bool core::eq(const core::StrView& a, const core::StrView& b);
template<> bool core::eq(const i32& a, const i32& b);
template<> bool core::eq(const u32& a, const u32& b);

namespace stlv {

// The memory subsystem is required everywhere, so it's definition is here.

enum struct AllocationType : u8 {
    UNKNOWN = 0,

    EVENT,

    SENTINEL
};

bool memInit();

void memDestroy();

void* memAlloc(addr_size size, AllocationType allocType = AllocationType::UNKNOWN) noexcept;

void* memCalloc(addr_size count, addr_size size) noexcept;

void memFree(void* ptr) noexcept;

addr_size memUsed() noexcept;

addr_size memTotalAllocated() noexcept;

} // namespace stlv
