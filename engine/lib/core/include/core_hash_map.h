#pragma once

#include <core_alloc.h>
#include <core_mem.h>
#include <core_traits.h>
#include <core_types.h>
#include <math/core_math.h>

namespace core {

namespace detail {

// The capacity of the hash_map is always a power of 2 to avoid using division in any of the hot code paths.
// This allows the use of a lookup table to determine when to resize the hash_map.
constexpr inline addr_size lookupMaxLoadFactor(addr_size cap) {
    switch (cap) {
        case 0: [[fallthrough]];
        case 1: [[fallthrough]];
        case 2: [[fallthrough]];
        case 4: [[fallthrough]];
        case 8: [[fallthrough]];
        case 16: return cap; // Smaller maps ^ use a factor of 1.0 to spare memory.
        case 32: return addr_size(32 * 0.80);
        case 64: return addr_size(64 * 0.80);
        case 128: return addr_size(128 * 0.80);
        case 256: return addr_size(256 * 0.80);
        case 512: return addr_size(512 * 0.80);
        case 1024: return addr_size(1024 * 0.80);
        case 2048: return addr_size(2048 * 0.80);
        case 4096: return addr_size(4096 * 0.80);
        case 8192: return addr_size(8192 * 0.80);
        case 16384: return addr_size(16384 * 0.80);
        case 32768: return addr_size(32768 * 0.80);
        case 65536: return addr_size(65536 * 0.80);
        case 131072: return addr_size(131072 * 0.80);
        case 262144: return addr_size(262144 * 0.80);
        case 524288: return addr_size(524288 * 0.80);
        case 1048576: return addr_size(1048576 * 0.80);
        case 2097152: return addr_size(2097152 * 0.80);
        case 4194304: return addr_size(4194304 * 0.80);
        case 8388608: return addr_size(8388608 * 0.80);
        case 16777216: return addr_size(16777216 * 0.80);
        case 33554432: return addr_size(33554432 * 0.80);
        case 67108864: return addr_size(67108864 * 0.80);
        case 134217728: return addr_size(134217728 * 0.80);
        case 268435456: return addr_size(268435456 * 0.80);
        case 536870912: return addr_size(536870912 * 0.80);
        case 1073741824: return addr_size(1073741824 * 0.80);
        case 2147483648: return addr_size(2147483648 * 0.80);
        case 4294967296: return addr_size(4294967296 * 0.80);
        case 8589934592: return addr_size(8589934592 * 0.80);
        case 17179869184: return addr_size(17179869184 * 0.80);
        case 34359738368: return addr_size(34359738368 * 0.80);
        case 68719476736: return addr_size(68719476736 * 0.80);
        case 137438953472: return addr_size(137438953472 * 0.80);
        case 274877906944: return addr_size(274877906944 * 0.80);
        case 549755813888: return addr_size(549755813888 * 0.80);
        case 1099511627776: return addr_size(1099511627776 * 0.80);
        case 2199023255552: return addr_size(2199023255552 * 0.80);
        case 4398046511104: return addr_size(4398046511104 * 0.80);
        case 8796093022208: return addr_size(8796093022208 * 0.80);
        case 17592186044416: return addr_size(17592186044416 * 0.80);
        case 35184372088832: return addr_size(35184372088832 * 0.80);
        case 70368744177664: return addr_size(70368744177664 * 0.80);
        case 140737488355328: return addr_size(140737488355328 * 0.80);
        case 281474976710656: return addr_size(281474976710656 * 0.80);
        case 562949953421312: return addr_size(562949953421312 * 0.80);
        case 1125899906842624: return addr_size(1125899906842624 * 0.80);
        case 2251799813685248: return addr_size(2251799813685248 * 0.80);
        case 4503599627370496: return addr_size(4503599627370496 * 0.80);
        case 9007199254740992: return addr_size(9007199254740992 * 0.80);
    }

    // slow path:
    Assert(false, "lookup_80percent: value is not a power of 2");
    return addr_size(f64(cap) * 0.80);
}

} // namespace detail

template <typename T> addr_size hash(const T& key) = delete;
template <typename T> bool eq(const T& a, const T& b) = delete;

template <typename T>
concept HashableConcept = requires {
    { core::hash<T>(core::declval<const T&>()) } -> core::same_as<addr_size>;
    { core::eq<T>(core::declval<const T&>(), core::declval<const T&>()) } -> core::same_as<bool>;
};

template<typename TKey, typename TValue, typename TAllocator = CORE_DEFAULT_ALLOCATOR()>
struct HashMap {
    static_assert(core::is_trivially_destructible_v<TKey>, "TKey must be trivially destructible");
    static_assert(HashableConcept<TKey>, "TKey must be hashable");

    using DataType       = TValue;
    using KeyType        = TKey;
    using SizeType       = addr_size;
    using AllocatorType  = TAllocator;
    using ContainerType  = HashMap<TKey, TValue, TAllocator>;

    struct Bucket {
        KeyType key;
        DataType data;
        bool occupied;
    };

    static constexpr SizeType DEFAULT_CAP = 16;
    static constexpr bool dataHasTrivialDestructor = core::is_trivially_destructible_v<DataType>;

    HashMap() : m_buckets(nullptr), m_cap(0), m_len(0) {};
    HashMap(SizeType cap) : m_buckets(nullptr), m_cap(core::alignToPow2(cap)), m_len(0) {
        m_buckets = reinterpret_cast<Bucket*>(AllocatorType::calloc(m_cap, sizeof(Bucket)));
    }
    HashMap(const ContainerType& other) = delete; // prevent copy ctor
    HashMap(ContainerType&& other) : m_buckets(other.m_buckets), m_cap(other.m_cap), m_len(other.m_len) {
        if (this == &other) return;
        other.m_buckets = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
    }
    ~HashMap() { free(); }

    ContainerType& operator=(const ContainerType& other) = delete; // prevent copy assignment
    ContainerType& operator=(SizeType) = delete; // prevent size assignment
    ContainerType& operator=(ContainerType&& other) {
        if (this == &other) return *this;
        free(); // very important to free before assigning new data.
        m_buckets = other.m_buckets;
        m_cap = other.m_cap;
        m_len = other.m_len;
        other.m_buckets = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
        return *this;
    }

    SizeType cap()     const { return m_cap; }
    SizeType byteCap() const { return m_cap * sizeof(Bucket); }
    SizeType len()     const { return m_len; }
    SizeType byteLen() const { return m_len * sizeof(Bucket); }
    bool     empty()   const { return m_len == 0; }

    void clear() {
        if constexpr (!dataHasTrivialDestructor) {
            for (SizeType i = 0; i < m_cap; ++i) {
                if (m_buckets[i].occupied) {
                    m_buckets[i].data.~DataType();
                    m_buckets[i].occupied = false;
                    m_buckets[i].key = KeyType();
                }
            }
        }
        else {
            core::memset(m_buckets, 0, m_cap * sizeof(Bucket));
        }

        m_len = 0;
    }

    void free() {
        if (m_buckets == nullptr) return;
        if constexpr (!dataHasTrivialDestructor) {
            // If the data is not trivially destructible call destructors manually:
            for (SizeType i = 0; i < m_cap; ++i) {
                if (m_buckets[i].occupied) {
                    m_buckets[i].data.~DataType();
                }
            }
        }
        m_len = 0;
        m_cap = 0;
        AllocatorType::free(m_buckets);
        m_buckets = nullptr;
    }

    ContainerType copy() {
        ContainerType cpy(m_cap);
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                cpy.put(m_buckets[i].key, m_buckets[i].data);
            }
        }
        return cpy;
    }

    const DataType* get(const KeyType& key) const {
        const Bucket* b = findBucket(key);
        return b ? &b->data : nullptr;
    }

    DataType* get(const KeyType& key) {
        return const_cast<DataType*>(static_cast<const ContainerType&>(*this).get(key));
    }

    ContainerType& put(const KeyType& key, const DataType& data) {
        return putImpl(key, data);
    }

    ContainerType& put(const KeyType& key, DataType&& data) {
        return putImpl(key, core::move(data));
    }

    ContainerType& remove(const KeyType& key) {
        Bucket* b = findBucket(key);
        if (b) {
            b->occupied = false;
            b->key = KeyType(); // rest key
            b->data = DataType(); // reset data
            m_len--;
        }
        return *this;
    }

    template <typename TKeyCb>
    inline const ContainerType& keys(TKeyCb cb) const {
        if (m_buckets == nullptr) return *this;
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                if(!cb(m_buckets[i].key)) {
                    break;
                }
            }
        }
        return *this;
    }

    template <typename TKeyCb>
    inline  ContainerType& keys(TKeyCb cb) {
        return const_cast<ContainerType&>(static_cast<const ContainerType&>(*this).keys(cb));
    }

    template <typename TDataCb>
    inline const ContainerType& values(TDataCb cb) const {
        if (m_buckets == nullptr) return *this;
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                if(!cb(m_buckets[i].data)) {
                    break;
                }
            }
        }
        return *this;
    }

    template <typename TDataCb>
    inline ContainerType& values(TDataCb cb) {
        return const_cast<ContainerType&>(static_cast<const ContainerType&>(*this).values(cb));
    }

    template <typename TKVCb>
    inline const ContainerType& kvs(TKVCb cb) const {
        if (m_buckets == nullptr) return *this;
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                if(!cb(m_buckets[i].key, m_buckets[i].data)) {
                    break;
                }
            }
        }
        return *this;
    }

    template <typename TKVCb>
    inline ContainerType& kvs(TKVCb cb) {
        return const_cast<ContainerType&>(static_cast<const ContainerType&>(*this).kvs(cb));
    }

private:

    template <typename Data>
    ContainerType& putImpl(const KeyType& key, Data&& data) {
        if (m_len >= detail::lookupMaxLoadFactor(m_cap)) {
            // Resize based on a load factor of 0.80
            resize();
        }

        auto h = hash(key);
        SizeType addr = SizeType(h) & (m_cap - 1);
        while (m_buckets[addr].occupied) {
            if (eq(m_buckets[addr].key, key)) {
                m_buckets[addr].data = core::move(data);
                return *this;
            }
            addr = (addr + 1) & (m_cap - 1);
        }
        m_buckets[addr].key = key;
        m_buckets[addr].data = core::move(data);
        m_buckets[addr].occupied = true;
        m_len++;

        return *this;
    }

    void resize() {
        SizeType newCap = m_cap < DEFAULT_CAP ? DEFAULT_CAP : m_cap * 2;
        HashMap newMap;
        newMap.m_buckets = reinterpret_cast<Bucket*>(AllocatorType::calloc(newCap, sizeof(Bucket)));
        newMap.m_cap = newCap;
        newMap.m_len = 0;

        if (m_buckets) {
            for (SizeType i = 0; i < m_cap; ++i) {
                if (m_buckets[i].occupied) {
                    newMap.put(m_buckets[i].key, core::move(m_buckets[i].data));
                }
            }
        }

        *this = core::move(newMap);
    }

    const Bucket* findBucket(const KeyType& key) const {
        auto h = hash(key);
        SizeType addr = SizeType(h) & (m_cap - 1);
        SizeType startAddr = addr;
        while (addr < m_cap && m_buckets[addr].occupied) {
            if (eq(m_buckets[addr].key, key)) {
                return &m_buckets[addr];
            }
            addr = (addr + 1) & (m_cap - 1);
            if (addr == startAddr) {
                // Full loop around the table.
                // This happens when the table is filled to capacity and the loop goes forever.
                // This is a problem only for small tables.
                break;
            }
        }
        return nullptr;
    }

    Bucket* findBucket(const KeyType& key) {
        return const_cast<Bucket*>(static_cast<const ContainerType&>(*this).findBucket(key));
    }

    Bucket* m_buckets;
    SizeType m_cap; // The capacity must always be a power of 2.
    SizeType m_len;
};

template<typename TKey, typename TAllocator = CORE_DEFAULT_ALLOCATOR()>
struct HashSet {
    static_assert(core::is_trivially_destructible_v<TKey>, "TKey must be trivially destructible");
    static_assert(HashableConcept<TKey>, "TKey must be hashable");

    using KeyType       = TKey;
    using SizeType      = addr_size;
    using AllocatorType = TAllocator;
    using ContainerType = HashSet<TKey, TAllocator>;

    struct Bucket {
        KeyType key;
        bool occupied;
    };

    constexpr static SizeType DEFAULT_CAP = 16;

    HashSet() : m_buckets(nullptr), m_cap(0), m_len(0) {};
    HashSet(SizeType cap) : m_buckets(nullptr), m_cap(core::alignToPow2(cap)), m_len(0) {
        m_buckets = reinterpret_cast<Bucket*>(AllocatorType::calloc(m_cap, sizeof(Bucket)));
    }
    HashSet(const ContainerType& other) = delete; // prevent copy ctor
    HashSet(ContainerType&& other) : m_buckets(other.m_buckets), m_cap(other.m_cap), m_len(other.m_len) {
        if (this == &other) return;
        other.m_buckets = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
    }
    ~HashSet() { free(); }

    HashSet& operator=(const ContainerType& other) = delete; // prevent copy assignment
    HashSet& operator=(SizeType) = delete; // prevent size assignment
    HashSet& operator=(ContainerType&& other) {
        if (this == &other) return *this;
        free(); // very important to free before assigning new data.
        m_buckets = other.m_buckets;
        m_cap = other.m_cap;
        m_len = other.m_len;
        other.m_buckets = nullptr;
        other.m_cap = 0;
        other.m_len = 0;
        return *this;
    }

    SizeType cap()     const { return m_cap; }
    SizeType byteCap() const { return m_cap * sizeof(Bucket); }
    SizeType len()     const { return m_len; }
    SizeType byteLen() const { return m_len * sizeof(Bucket); }
    bool     empty()  const { return m_len == 0; }

    void clear() {
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                m_buckets[i].occupied = false;
                m_buckets[i].key = KeyType();
            }
        }
        m_len = 0;
    }

    void free() {
        if (m_buckets == nullptr) return;
        m_len = 0;
        m_cap = 0;
        AllocatorType::free(m_buckets);
        m_buckets = nullptr;
    }

    ContainerType copy() {
        ContainerType cpy(m_cap);
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                cpy.put(m_buckets[i].key);
            }
        }
        return cpy;
    }

    const KeyType* get(const KeyType& key) const {
        const Bucket* b = findBucket(key);
        return b ? &b->key : nullptr;
    }

    KeyType* get(const KeyType& key) {
        return const_cast<KeyType*>(static_cast<const ContainerType &>(*this).get(key));
    }

    ContainerType& put(const KeyType& key) {
        if (m_len >= detail::lookupMaxLoadFactor(m_cap)) {
            // Resize based on a load factor of 0.80
            resize();
        }

        auto h = hash(key);
        SizeType addr = SizeType(h) & (m_cap - 1);
        while (m_buckets[addr].occupied) {
            if (eq(m_buckets[addr].key, key)) {
                return *this;
            }
            addr = (addr + 1) & (m_cap - 1);
        }
        m_buckets[addr].key = key;
        m_buckets[addr].occupied = true;
        m_len++;

        return *this;
    }

    ContainerType& remove(const KeyType& key) {
        Bucket* b = findBucket(key);
        if (b) {
            b->occupied = false;
            b->key = KeyType(); // rest key
            m_len--;
        }
        return *this;
    }

    template <typename TKeyCb>
    inline const ContainerType& keys(TKeyCb cb) const {
        if (m_buckets == nullptr) return *this;
        for (SizeType i = 0; i < m_cap; ++i) {
            if (m_buckets[i].occupied) {
                if(!cb(m_buckets[i].key)) {
                    break;
                }
            }
        }
        return *this;
    }

    template <typename TKeyCb>
    inline ContainerType& keys(TKeyCb cb) {
        return const_cast<ContainerType&>(static_cast<const ContainerType&>(*this).keys(cb));
    }

private:

    void resize() {
        SizeType newCap = m_cap < DEFAULT_CAP ? DEFAULT_CAP : m_cap * 2;
        ContainerType newSet;
        newSet.m_buckets = reinterpret_cast<Bucket*>(AllocatorType::calloc(newCap, sizeof(Bucket)));
        newSet.m_cap = newCap;
        newSet.m_len = 0;

        if (m_buckets) {
            for (SizeType i = 0; i < m_cap; ++i) {
                if (m_buckets[i].occupied) {
                    newSet.put(m_buckets[i].key);
                }
            }
        }

        *this = core::move(newSet);
    }

    const Bucket* findBucket(const KeyType& key) const {
        auto h = hash(key);
        SizeType addr = SizeType(h) & (m_cap - 1);
        SizeType startAddr = addr;
        while (addr < m_cap && m_buckets[addr].occupied) {
            if (eq(m_buckets[addr].key, key)) {
                return &m_buckets[addr];
            }
            addr = (addr + 1) & (m_cap - 1);
            if (addr == startAddr) {
                // Full loop around the table.
                // This happens when the table is filled to capacity and the loop goes forever.
                // This is a problem only for small tables.
                break;
            }
        }
        return nullptr;
    }

    Bucket* findBucket(const KeyType& key) {
        return const_cast<Bucket*>(static_cast<const ContainerType&>(*this).findBucket(key));
    }

    Bucket*  m_buckets;
    SizeType m_cap; // The capacity must always be a power of 2.
    SizeType m_len;
};

} // namespace core
