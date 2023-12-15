#pragma once

#include <core_arr.h>
#include <core_traits.h>
#include <core_types.h>
#include <math/core_math.h>
#include <math/core_matrix.h>
#include <math/core_vec.h>

namespace core {

using namespace coretypes;

// Translate

template<addr_size Dim, typename T>
constexpr vec<Dim, T> translate(const vec<Dim, T>& v, const vec<Dim, T>& t) {
    vec<Dim, T> ret = {};
    for (addr_size i = 0; i < v.dimensions(); ++i) {
        ret[i] = v[i] + t[i];
    }
    return ret;
}

template<typename T>
constexpr mat4<T> translate(const mat4<T>& m, const vec3<T>& t) {
    mat4<T> ret = m;
    ret[3][0] += t[0];
    ret[3][1] += t[1];
    ret[3][2] += t[2];
    return ret;
}

// Scale

template<addr_size Dim, typename T>
constexpr vec<Dim, T> scale(const vec<Dim, T>& v, const vec<Dim, T>& s) {
    vec<Dim, T> ret = {};
    for (addr_size i = 0; i < v.dimensions(); ++i) {
        ret[i] = v[i] * s[i];
    }
    return ret;
}

template<addr_size Dim, typename T>
constexpr vec<Dim, T> scale(const vec<Dim, T>& v, const vec<Dim, T>& axis, const vec<Dim, T>& s) {
    vec<Dim, T> ret = {};
    for (addr_size i = 0; i < v.dimensions(); ++i) {
        ret[i] = axis[i] + (v[i] - axis[i]) * s[i];
    }
    return ret;
}

template<typename T>
constexpr mat4<T> scale(const mat4<T>& m, const vec3<T>& s) {
    auto ret = m;
    ret[0][0] *= s[0]; ret[0][1] *= s[0]; ret[0][2] *= s[0];
    ret[1][0] *= s[1]; ret[1][1] *= s[1]; ret[1][2] *= s[1];
    ret[2][0] *= s[2]; ret[2][1] *= s[2]; ret[2][2] *= s[2];
    return ret;
}

// Rotate 2D

template<typename TFloat>
inline vec2<TFloat> rotate(const vec2<TFloat>& v, const vec2<TFloat>& origin, core::radians angle) {
    static_assert(core::is_float_v<TFloat>, "type must be floating point");
    vec2<TFloat> ret = v;
    TFloat c = core::cos(f32(angle));
    TFloat s = core::sin(f32(angle));
    ret -= origin;
    ret = core::v<TFloat>(ret.x() * c - ret.y() * s, ret.x() * s + ret.y() * c);
    ret += origin;
    return ret;
}

template<typename TFloat>
inline vec2<TFloat> rotateRight(const vec2<TFloat>& v, const vec2<TFloat>& origin, core::radians angle) {
    angle.value = -angle.value;
    return rotate(v, origin, angle);
}

// Rotate 3D

template<typename TFloat>
inline mat4<TFloat> rotate(const mat4<TFloat>& m, const vec3<TFloat>& a, core::radians angle) {
    static_assert(core::is_float_v<TFloat>, "type must be floating point");

    TFloat c = core::cos(f32(angle));
    TFloat s = core::sin(f32(angle));
    TFloat t = TFloat(1) - c;

    vec3<TFloat> axis = a.norm(); // I could assume that the axis is always normalized to avoid this call.

    auto m00 = t * axis.x() * axis.x() + c;
    auto m10 = t * axis.x() * axis.y() - s * axis.z();
    auto m20 = t * axis.x() * axis.z() + s * axis.y();
    auto m01 = t * axis.x() * axis.y() + s * axis.z();
    auto m11 = t * axis.y() * axis.y() + c;
    auto m21 = t * axis.y() * axis.z() - s * axis.x();
    auto m02 = t * axis.x() * axis.z() - s * axis.y();
    auto m12 = t * axis.y() * axis.z() + s * axis.x();
    auto m22 = t * axis.z() * axis.z() + c;

    mat4<TFloat> r (
        m00, m10, m20, 0,
        m01, m11, m21, 0,
        m02, m12, m22, 0,
          0,   0,   0, 1
    );

    auto ret = r * m;
    return ret;
}

template<typename TFloat>
inline mat4<TFloat> rotateRight(const mat4<TFloat>& v, const vec3<TFloat>& axis, core::radians angle) {
    angle.value = -angle.value;
    return rotate(v, axis, angle);
}

template<typename TFloat>
inline mat4<TFloat> rotateX(const mat4<TFloat>& m, core::radians angle) {
    static_assert(core::is_float_v<TFloat>, "type must be floating point");

    TFloat c = core::cos(f32(angle));
    TFloat s = core::sin(f32(angle));

    mat4<TFloat> r (
        1, 0, 0, 0,
        0, c, s, 0,
        0,-s, c, 0,
        0, 0, 0, 1
    );

    auto ret = r * m;
    return ret;
}

template<typename TFloat>
inline mat4<TFloat> rotateXRight(const mat4<TFloat>& m, core::radians angle) {
    angle.value = -angle.value;
    return rotateX(m, angle);
}

template<typename TFloat>
inline mat4<TFloat> rotateY(const mat4<TFloat>& m, core::radians angle) {
    static_assert(core::is_float_v<TFloat>, "type must be floating point");

    TFloat c = core::cos(f32(angle));
    TFloat s = core::sin(f32(angle));

    mat4<TFloat> r (
        c, 0,-s, 0,
        0, 1, 0, 0,
        s, 0, c, 0,
        0, 0, 0, 1
    );

    auto ret = r * m;
    return ret;
}

template<typename TFloat>
inline mat4<TFloat> rotateYRight(const mat4<TFloat>& m, core::radians angle) {
    angle.value = -angle.value;
    return rotateY(m, angle);
}

template<typename TFloat>
inline mat4<TFloat> rotateZ(const mat4<TFloat>& m, core::radians angle) {
    static_assert(core::is_float_v<TFloat>, "type must be floating point");

    TFloat c = core::cos(f32(angle));
    TFloat s = core::sin(f32(angle));

    mat4<TFloat> r (
        c, s, 0, 0,
       -s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    auto ret = r * m;
    return ret;
}

template<typename TFloat>
inline mat4<TFloat> rotateZRight(const mat4<TFloat>& m, core::radians angle) {
    angle.value = -angle.value;
    return rotateZ(m, angle);
}

}
