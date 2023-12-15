#pragma once

#include <core_types.h>
#include <core_alloc.h>
#include <core_traits.h>

namespace core {

template <typename T, typename TAllocator = CORE_DEFAULT_ALLOCATOR()>
struct UniquePtr {
    using DataType = T;
    using AllocatorType = TAllocator;
    using ContainerType = UniquePtr<T, TAllocator>;

    UniquePtr() : m_ptr(nullptr) {}
    UniquePtr(DataType* newPtr) : m_ptr(newPtr) {}
    UniquePtr(ContainerType&& other) {
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
    }
    ContainerType& operator=(ContainerType&& other) {
        if (this == &other) return *this;
        reset();
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    // no copy
    NO_COPY(UniquePtr);

    ~UniquePtr() { reset(); }

    ContainerType copy() const {
        ContainerType ret;
        ret.m_ptr = AllocatorType::template construct<DataType>(*m_ptr);
        return ret;
    }

    void reset(DataType* newPtr = nullptr) {
        if (m_ptr) {
            if constexpr (!core::is_trivially_destructible_v<DataType>) {
                m_ptr->~T();
            }
            AllocatorType::free(m_ptr);
        }
        m_ptr = newPtr;
    }

    DataType* steal() {
        DataType* ret = m_ptr;
        m_ptr = nullptr;
        return ret;
    }

    void swap(ContainerType& other) {
        DataType* tmp = m_ptr;
        m_ptr = other.m_ptr;
        other.m_ptr = tmp;
    }

    DataType* operator->() const { return m_ptr; }
    DataType& operator*() const { return *m_ptr; }

    DataType* get() const { return m_ptr; }

    explicit operator bool() const { return m_ptr != nullptr; }

    DataType& operator[](addr_size idx) const { return m_ptr[idx]; }

private:
    DataType* m_ptr;
};

template <typename T, typename TAllocator = CORE_DEFAULT_ALLOCATOR(), typename... Args>
UniquePtr<T, TAllocator> makeUnique(Args&&... args) {
    T* rawPtr = TAllocator::template construct<T>(args...);
    UniquePtr<T, TAllocator> ret (rawPtr);
    return ret;
}

} // namespace core
