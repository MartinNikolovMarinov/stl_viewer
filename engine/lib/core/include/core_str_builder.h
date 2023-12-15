#pragma once

#include <core_alloc.h>
#include <core_cptr.h>
#include <core_traits.h>
#include <core_types.h>
#include <core_mem.h>

namespace core {

using namespace coretypes;

struct StrView {
    using DataType = char;
    using SizeType = addr_size;

    const DataType* buff;
    SizeType length;

    constexpr bool eq(const StrView& other) const {
        return core::cptrCmp(buff, length, other.buff, other.length) == 0;
    }

    constexpr const DataType* data() const { return buff; }
    constexpr SizeType len() const { return length; }

    constexpr operator bool() const { return buff != nullptr; }

    constexpr const DataType& operator[](SizeType idx) const { return buff[idx]; }
};

constexpr StrView sv()                               { return {nullptr, 0}; }
constexpr StrView sv(const char* str)                { return {str, core::cptrLen(str)}; }
constexpr StrView sv(const char* str, addr_size len) { return {str, len}; }

static_assert(core::is_pod_v<StrView>, "StrView must be pod type");

// TODO2: [PERFORMANCE] Might want to do some small string optimization.
//        Also, the JIT null termination seems a bit stupid now.

template <typename TAllocator = CORE_DEFAULT_ALLOCATOR()>
struct StrBuilder {
    using DataType      = char;
    using SizeType      = addr_size;
    using AllocatorType = TAllocator;
    using ContainerType = StrBuilder<AllocatorType>;

    StrBuilder() : m_data(nullptr), m_cap(0), m_len(0) {}
    StrBuilder(const ContainerType&) = delete; // prevent copy ctor
    StrBuilder(ContainerType&& other) : m_data(other.m_data), m_cap(other.m_cap), m_len(other.m_len) {
        if (this == &other) return;
        other.m_data = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
    }

    StrBuilder(SizeType len) : m_data(nullptr), m_cap(len + 1), m_len(len) {
        m_data = reinterpret_cast<DataType*>(AllocatorType::alloc(m_cap * sizeof(DataType)));
        Assert(m_data != nullptr);
        core::memset(m_data, 0, m_len * sizeof(DataType));
    }
    StrBuilder(SizeType len, SizeType cap) : m_data(nullptr), m_cap(cap), m_len(len) {
        Assert(m_cap >= m_len);
        if (m_cap == m_len) m_cap++; // +1 for null terminator
        m_data = reinterpret_cast<DataType*>(AllocatorType::alloc(m_cap * sizeof(DataType)));
        Assert(m_data != nullptr);
        core::memset(m_data, 0, m_len * sizeof(DataType));
    }

    StrBuilder(const DataType* cptr) { assignFromCptr(cptr, core::cptrLen(cptr)); }
    StrBuilder(StrView view)         { assignFromCptr(view.data(), view.len()); }

    ~StrBuilder() { free(); }

    ContainerType& operator=(const ContainerType&) = delete; // prevent copy assignment
    ContainerType& operator=(SizeType) = delete; // prevent len assignment
    ContainerType& operator=(ContainerType&& other) {
        if (this == &other) return *this;
        free(); // very important to free before assigning new data.
        m_data = other.m_data;
        m_cap = other.m_cap;
        m_len = other.m_len;
        other.m_data = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
        return *this;
    }
    ContainerType& operator=(const DataType* cptr) {
        clear();
        return append(cptr, core::cptrLen(cptr));
    }
    ContainerType& operator=(StrView view) {
        clear();
        return append(view.buff, view.len());
    }

    SizeType cap()     const { return m_cap; }
    SizeType byteCap() const { return m_cap * sizeof(DataType); }
    SizeType len()     const { return m_len; }
    SizeType byteLen() const { return m_len * sizeof(DataType); }
    bool     empty()   const { return m_len == 0; }

    StrView view() const {
        if (m_data != nullptr) m_data[m_len] = core::term_char; // JIT null terminate
        return {m_data, m_len};
    }

    bool eq(const ContainerType& other) const {
        return view().eq(other.view());
    }

    bool eq(const StrView& other) const {
        return view().eq(other);
    }

    void clear() { m_len = 0; }

    void free() {
        m_len = 0;
        m_cap = 0;
        if (m_data != nullptr) {
            AllocatorType::free(m_data);
            m_data = nullptr;
        }
    }

    ContainerType copy() const {
        ContainerType copy(m_len, m_cap);
        core::memcopy(copy.m_data, m_data, m_len * sizeof(DataType));
        return copy;
    }

    DataType& at(SizeType idx)                     { Assert(idx < m_len); return m_data[idx]; }
    const DataType& at(SizeType idx)         const { Assert(idx < m_len); return m_data[idx]; }
    DataType& operator[](SizeType idx)             { return at(idx); }
    const DataType& operator[](SizeType idx) const { return at(idx); }

    DataType& first()             { return at(0); }
    const DataType& first() const { return at(0); }
    DataType& last()              { return at(m_len - 1); }
    const DataType& last()  const { return at(m_len - 1); }

    void reset(DataType** ptr) {
        free();
        m_data = reinterpret_cast<DataType*>(ptr ? *ptr : nullptr);
        m_len = core::cptrLen(ptr ? *ptr : nullptr);
        m_cap = m_len + 1; // +1 for null terminator
        *ptr = nullptr;
    }

    DataType* steal() {
        if (m_data != nullptr) m_data[m_len] = core::term_char; // JIT null terminate
        DataType* res = m_data;
        m_data = nullptr;
        m_cap = 0;
        m_len = 0;
        return res;
    }

    ContainerType& append(const DataType& val) {
        if (shouldResize(1)) {
            adjustCap(m_cap == 0 ? 2 : m_cap * 2);
        }
        m_data[m_len] = val;
        m_len++;
        return *this;
    }

    ContainerType& append(const DataType* cptr, SizeType len) {
        if (len == 0) return *this;
        if (shouldResize(len)) {
            adjustCap(m_cap <= len ? (m_cap*2 + len + 1) : m_cap * 2);
        }
        core::cptrCopy(m_data + m_len, cptr, len);
        m_len += len;
        return *this;
    }

    ContainerType& append(const DataType* cptr) {
        return append(cptr, core::cptrLen(cptr));
    }

    ContainerType& append(StrView view) {
        return append(view.buff, view.len());
    }

    ContainerType& fill(const DataType& val) {
        if (m_data != nullptr) core::memset(m_data, u8(val), m_len * sizeof(DataType));
        return *this;
    }

    void adjustCap(SizeType newCap) {
        if (newCap <= m_cap) {
            // shrink
            m_len = m_len > newCap ? newCap - 1 : m_len;
            m_cap = newCap;
            return;
        }

        // reallocate
        DataType* newData = reinterpret_cast<DataType *>(AllocatorType::alloc(newCap * sizeof(DataType)));
        Assert(newData != nullptr);
        if (m_data != nullptr) {
            core::memcopy(newData, m_data, m_len * sizeof(DataType));
            AllocatorType::free(m_data);
        }
        m_data = newData;
        m_cap = newCap;
    }

private:
    DataType* m_data;
    SizeType  m_cap;
    SizeType  m_len;

    inline void assignFromCptr(const char* cptr, SizeType len) {
        m_len = len;
        m_cap = m_len + 1; // +1 for null terminator
        m_data = reinterpret_cast<DataType*>(AllocatorType::alloc(m_cap * sizeof(DataType)));
        Assert(m_data != nullptr);
        core::cptrCopy(m_data, cptr, m_len);
        m_data[m_cap - 1] = core::term_char;
    }

    inline bool shouldResize(SizeType lenInc) {
        constexpr SizeType nullTerminatorSize = 1;
        return (m_len + lenInc + nullTerminatorSize > m_cap);
    }
};

static_assert(core::is_standard_layout_v<StrBuilder<>>);

} // namespace core
