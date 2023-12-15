#pragma once

#include <core_traits.h>
#include <core_types.h>
#include <math/core_math.h>
#include <math/core_vec.h>

namespace core {

using namespace coretypes;

template <typename TFloat>
inline TFloat slope(TFloat x1, TFloat y1, TFloat x2, TFloat y2) {
    static_assert(core::is_float_v<TFloat>, "Invalid TFloat type.");
    TFloat raise = y2 - y1;
    TFloat run = x2 - x1;
    auto ret =  core::atan2(raise, run);
    return ret;
}

template <typename TFloat>
inline TFloat slope(const vec2<TFloat>& a, const vec2<TFloat>& b) {
    return slope(a.x(), a.y(), b.x(), b.y());
}

/**
 * @brief Finds the intersection point of two lines.
 *        It's important to note that this function works with lines not linear segments.
 *
 * @param a First point of the first line.
 * @param b Second point of the first line.
 * @param c First point of the second line.
 * @param d Second point of the second line.
 * @param out The point of intersection.
 *
 * @return True if the lines intersect, false otherwise.
 **/
template <typename TFloat>
constexpr inline bool lineToLineIntersection(const core::vec2<TFloat>& a, const core::vec2<TFloat>& b,
                                                const core::vec2<TFloat>& c, const core::vec2<TFloat>& d,
                                                core::vec2<TFloat>& out) {
    static_assert(core::is_float_v<TFloat>, "Invalid TFloat type.");
    // Formula of line to line intersection.
    TFloat x1 = a.x(); TFloat y1 = a.y();
    TFloat x2 = b.x(); TFloat y2 = b.y();
    TFloat x3 = c.x(); TFloat y3 = c.y();
    TFloat x4 = d.x(); TFloat y4 = d.y();
    f32 dnom = (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4);
    if (dnom == 0) return false; // Parallel or coincident lines.
    TFloat t = ((x1-x3)*(y3-y4) - (y1-y3)*(x3-x4)) / dnom;
    out = core::v(x1 + t*(x2-x1), y1 + t*(y2-y1));
    return true;
}
template <typename TFloat>
constexpr inline bool lineToLineIntersection(const core::vec2<TFloat>& a, const core::vec2<TFloat>& b,
                                                const core::vec2<TFloat>& c, const core::vec2<TFloat>& d,
                                                TFloat& x, TFloat& y) {
    auto t = core::v(x, y);
    bool ret = lineToLineIntersection(a, b, c, d, t);
    x = t.x();
    y = t.y();
    return ret;
}
template <typename TFloat>
constexpr  inline bool lineToLineIntersection(TFloat x1, TFloat y1, TFloat x2, TFloat y2,
                                                 TFloat x3, TFloat y3, TFloat x4, TFloat y4,
                                                 TFloat& x, TFloat& y) {
    auto t = core::v(x, y);
    bool ret = lineToLineIntersection(core::v(x1, y1), core::v(x2, y2), core::v(x3, y3), core::v(x4, y4), t);
    x = t.x();
    y = t.y();
    return ret;
}

} // namespace core
