#pragma once

#include <core_alloc.h>
#include <core_mem.h>
#include <core_traits.h>
#include <core_types.h>
#include <math/core_math.h>

namespace core {

using namespace coretypes;

template<typename T, typename TAllocator = CORE_DEFAULT_ALLOCATOR()>
struct Arr {
    using DataType      = T;
    using SizeType      = addr_size;
    using AllocatorType = TAllocator;
    using ContainerType = Arr<T, TAllocator>;

    static constexpr bool dataIsTrivial = core::is_trivial_v<DataType>;
    static constexpr bool dataHasTrivialDestructor = core::is_trivially_destructible_v<DataType>;

    Arr() : m_data(nullptr), m_cap(0), m_len(0) {}
    Arr(SizeType len) : m_data(nullptr), m_cap(len), m_len(len) {
        if (m_cap > 0) {
            callDefaultCtorsIfTypeIsNonTrivial();
        }
    }
    Arr(SizeType len, SizeType cap) : m_data(nullptr), m_cap(cap), m_len(len) {
        Assert(m_cap >= m_len);
        if (m_cap > 0) {
            callDefaultCtorsIfTypeIsNonTrivial();
        }
    }
    Arr(const ContainerType& other) = delete; // prevent copy ctor
    Arr(ContainerType&& other) : m_data(other.m_data), m_cap(other.m_cap), m_len(other.m_len) {
        if (this == &other) return;
        other.m_data = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
    }
    ~Arr() { free(); }

    ContainerType& operator=(const ContainerType& other) = delete; // prevent copy assignment
    ContainerType& operator=(SizeType) = delete; // prevent size assignment
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

    SizeType        cap()     const { return m_cap; }
    SizeType        byteCap() const { return m_cap * sizeof(DataType); }
    SizeType        len()     const { return m_len; }
    SizeType        byteLen() const { return m_len * sizeof(DataType); }
    DataType*       data()          { return m_data; }
    const DataType* data()    const { return m_data; }
    bool            empty()   const { return m_len == 0; }

    void clear() {
        if constexpr (!dataHasTrivialDestructor) {
            // For elements that are not trivially destructible call destructors manually:
            for (SizeType i = 0; i < m_len; ++i) {
                m_data[i].~T();
            }
        }
        m_len = 0;
    }

    ContainerType copy() {
        DataType* dataCopy = nullptr;
        if (m_cap > 0) {
            dataCopy = reinterpret_cast<DataType *>(AllocatorType::alloc(m_cap * sizeof(DataType)));
            Assert(dataCopy != nullptr);
            if constexpr (dataIsTrivial) {
                core::memcopy(dataCopy, m_data, m_len * sizeof(DataType));
            }
            else {
                for (SizeType i = 0; i < m_len; i++) {
                    auto& rawBytes = dataCopy[i];
                    new (reinterpret_cast<void*>(&rawBytes)) DataType(m_data[i]);
                }
            }
        }
        ContainerType result;
        result.m_data = dataCopy;
        result.m_cap = m_cap;
        result.m_len = m_len;
        return result;
    }

    void free() {
        if (m_data == nullptr) return;
        clear();
        m_cap = 0;
        AllocatorType::free(m_data);
        m_data = nullptr;
    }

    DataType& at(SizeType idx)                     { Assert(idx < m_len); return m_data[idx]; }
    const DataType& at(SizeType idx)         const { Assert(idx < m_len); return m_data[idx]; }
    DataType& operator[](SizeType idx)             { return at(idx); }
    const DataType& operator[](SizeType idx) const { return at(idx); }

    DataType& first()             { return at(0); }
    const DataType& first() const { return at(0); }
    DataType& last()              { return at(m_len - 1); }
    const DataType& last()  const { return at(m_len - 1); }

    ContainerType& append(const DataType& val) {
        if (m_len == m_cap) {
            adjustCap(m_cap == 0 ? 1 : m_cap * 2);
        }
        copyDataAt(val, m_len);
        m_len++;
        return *this;
    }

    ContainerType& append(DataType&& val) {
        if (m_len == m_cap) {
            adjustCap(m_cap == 0 ? 1 : m_cap * 2);
        }
        stealDataAt(core::move(val), m_len);
        m_len++;
        return *this;
    }

    ContainerType& append(const DataType* val, SizeType len) {
        if (m_len + len > m_cap) {
            adjustCap(m_cap <= len ? (m_cap*2 + len) : m_cap * 2);
        }
        copyDataAt(val, m_len, len);
        m_len += len;
        return *this;
    }

    ContainerType& append(const ContainerType& other) {
        return append(other.m_data, other.m_len);
    }

    ContainerType& fill(const DataType& val, addr_size from, addr_size to) {
        if (from >= to) return *this;

        [[maybe_unused]] addr_size overwriteIdx = core::min(to, m_len);
        if (to > m_len) {
            adjustCap(to);
            m_len = to;
        }

        addr_size count = to - from;

        if constexpr (dataIsTrivial) {
            core::memfill(m_data + from, count, val);
        }
        else {
            if constexpr (!dataHasTrivialDestructor) {
                // Destroy only the elements that are being overwritten:
                for (SizeType i = from; i < overwriteIdx; ++i) {
                    m_data[i].~T();
                }
            }

            for (SizeType i = from; i < from + count; ++i) {
                copyDataAt(val, i);
            }
        }

        return *this;
    }

    ContainerType& remove(SizeType idx) {
        Assert(idx < m_len);
        if constexpr (!dataHasTrivialDestructor) {
            // For elements that are not trivially destructible, call destructors manually:
            m_data[idx].~T();
        }
        if (idx < m_len - 1) {
            for (SizeType i = idx; i < m_len - 1; ++i) {
                m_data[i] = m_data[i + 1];
            }
        }
        m_len--;
        return *this;
    }

    // This is the "I know what I am doing" method. Gives ownership of data to the array. The array's destructor will
    // free it, so if a different allocator was used to allocate the data the results are undefined.
    void reset(DataType* data, SizeType len) {
        free();
        m_data = data;
        m_cap = len;
        m_len = len;
    }

    void adjustCap(SizeType newCap) {
        if (newCap <= m_cap) {
            if constexpr (dataIsTrivial) {
                m_len = m_len > newCap ? newCap : m_len;
            }
            else {
                // For elements that are not trivially destructible call destructors manually:
                for (SizeType i = newCap; i < m_len; ++i) {
                    m_data[i].~T();
                }
                m_len = m_len > newCap ? newCap : m_len;
            }
            m_cap = newCap;
            return;
        }

        // reallocate
        DataType* newData = reinterpret_cast<DataType *>(AllocatorType::alloc(newCap * sizeof(DataType)));
        // NOTE: Choosing not to clear the new memory here might bite me on the ass later.
        Assert(newData != nullptr);
        if (m_data != nullptr) {
            core::memcopy(newData, m_data, m_len * sizeof(DataType));
            AllocatorType::free(m_data);
        }
        m_data = newData;
        m_cap = newCap;
    }

private:
    DataType *m_data;
    SizeType m_cap;
    SizeType m_len;

    inline void stealDataAt(DataType&& rval, SizeType pos) {
        auto& rawBytes = m_data[pos];
        new (reinterpret_cast<void*>(&rawBytes)) DataType(core::move(rval));
    }

    inline void copyDataAt(const DataType& lval, SizeType pos) {
        if constexpr (dataIsTrivial) {
            m_data[pos] = lval;
        }
        else {
            auto& rawBytes = m_data[pos];
            new (reinterpret_cast<void*>(&rawBytes)) DataType(lval);
        }
    }

    inline void copyDataAt(const DataType* pval, SizeType pos, SizeType len) {
        if constexpr (dataIsTrivial) {
            core::memcopy(m_data + pos, pval, len * sizeof(DataType));
        }
        else {
            for (SizeType i = 0; i < len; i++) {
                auto& rawBytes = m_data[i + pos];
                new (reinterpret_cast<void*>(&rawBytes)) DataType(pval[i]);
            }
        }
    }

    inline void callDefaultCtorsIfTypeIsNonTrivial() {
        // This is exactly the same as calling fill(T()) but it has slightly less overhead.
        if constexpr (!dataIsTrivial) {
            m_data = reinterpret_cast<DataType *>(AllocatorType::alloc(m_cap * sizeof(DataType)));
            Assert(m_data != nullptr);
            for (SizeType i = 0; i < m_len; ++i) {
                new (&m_data[i]) DataType();
            }
        }
        else {
            m_data = reinterpret_cast<DataType *>(AllocatorType::calloc(m_cap, sizeof(DataType)));
            Assert(m_data != nullptr);
        }
    }
};

static_assert(core::is_standard_layout_v<Arr<i32>>, "Arr<i32> must be standard layout");

} // namespace core
