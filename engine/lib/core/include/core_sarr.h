#pragma once

#include <core_types.h>
#include <core_mem.h>
#include <core_utils.h>

namespace core {

using namespace coretypes;

template <typename T, addr_size N>
struct SArr {
    using DataType = T;
    using SizeType = addr_size;
    using ContainerType = SArr<T, N>;

    static constexpr bool dataIsStandardLayout = core::is_standard_layout_v<DataType>;
    static_assert(dataIsStandardLayout, "SArr data type must be standard layout");

    // trivial copy constructor, trivial copy assignment operator

    constexpr SArr() : m_data(), m_len(0) {}
    constexpr SArr(SizeType len) : m_data(), m_len(len) { fill(DataType(), 0, m_len); Assert(m_len <= N); }
    constexpr SArr(const ContainerType& other) = delete; // prevent copy ctor
    constexpr SArr(ContainerType&& other) = default;

    constexpr ContainerType& operator=(const ContainerType& other) = delete; // Prevent copy assignment
    constexpr ContainerType& operator=(ContainerType&& other) = default;

    constexpr ContainerType copy() const {
        ContainerType ret;
        for (SizeType i = 0; i < m_len; ++i) ret.append(m_data[i]);
        return ret;
    }

    constexpr SizeType        cap()     const { return N; }
    constexpr SizeType        byteCap() const { return N * sizeof(DataType); }
    constexpr SizeType        len()     const { return m_len; }
    constexpr SizeType        byteLen() const { return m_len * sizeof(DataType); }
    constexpr DataType*       data()          { return m_data; }
    constexpr const DataType* data()    const { return m_data; }
    constexpr bool            empty()   const { return m_len == 0; }
    constexpr void            clear()         { m_len = 0; }

    constexpr DataType& at(SizeType idx)                     { return m_data[idx]; }
    constexpr const DataType& at(SizeType idx)         const { return m_data[idx]; }
    constexpr DataType& operator[](SizeType idx)             { return at(idx); }
    constexpr const DataType& operator[](SizeType idx) const { return at(idx); }

    constexpr DataType& first()             { return at(0); }
    constexpr const DataType& first() const { return at(0); }
    constexpr DataType& last()              { return at(m_len - 1); }
    constexpr const DataType& last()  const { return at(m_len - 1); }

    constexpr ContainerType& append(const DataType& val) {
        m_data[m_len] = val;
        m_len++;
        return *this;
    }

    constexpr ContainerType& append(DataType&& val) {
        m_data[m_len] = core::move(val);
        m_len++;
        return *this;
    }

    constexpr ContainerType& append(const ContainerType& other) {
        for (SizeType i = 0; i < other.m_len; ++i) append(other.m_data[i]);
        return *this;
    }

    constexpr ContainerType& fill(const DataType& val, SizeType from, SizeType to) {
        if (from >= m_len) return *this;
        if (to > N) to = N;
        if (to > m_len) m_len = to;
        for (SizeType i = from; i < to; ++i) m_data[i] = val;
        return *this;
    }

    constexpr ContainerType& remove(SizeType idx) {
        Assert(idx < m_len);
        for (SizeType i = idx; i < m_len - 1; ++i) {
            m_data[i] = m_data[i + 1];
        }
        m_len--;
        return *this;
    }

private:
    DataType m_data[N];
    SizeType m_len;
};

namespace detail {

template<typename TArg, typename ...Args>
constexpr auto _create_sarr(TArg first, Args... rest) {
    auto ret = SArr<TArg, (sizeof...(rest) + 1)>();
    ret.append(first);
    auto f = [&](auto arg) { ret.append(arg); };
    (f(rest), ...);
    return ret;
}

};

template<typename ...Args>
constexpr auto create_sarr(Args... args) {
    static_assert(sizeof...(Args) > 0, "create_sarr requires at least one argument");
    return detail::_create_sarr(args...);
}

} // namespace core
