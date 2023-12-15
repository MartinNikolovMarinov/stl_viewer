#pragma once

#include <core_API.h>
#include <core_types.h>
#include <math/core_math.h>
#include <math/core_matrix.h>
#include <math/core_vec.h>

namespace core {

using namespace coretypes;

/**
 * @brief Creates a right-handed orthographic matrix with a near plane of -1 and a far plane of 1.
 *
 * @param left The left plane.
 * @param right The right plane.
 * @param bottom The bottom plane.
 * @param top The top plane.
 *
 * @return The orthographic matrix.
*/
CORE_API_EXPORT mat4x4f orthoRH_NO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

/**
 * @brief Creates a right-handed orthographic matrix with a near plane of 0 and a far plane of 1.
 *
 * @param left The left plane.
 * @param right The right plane.
 * @param bottom The bottom plane.
 * @param top The top plane.
 *
 * @return The orthographic matrix.
*/
CORE_API_EXPORT mat4x4f orthoRH_ZO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

/**
 * @brief Creates a left-handed orthographic matrix with a near plane of -1 and a far plane of 1.
 *
 * @param left The left plane.
 * @param right The right plane.
 * @param bottom The bottom plane.
 * @param top The top plane.
 *
 * @return The orthographic matrix.
*/
CORE_API_EXPORT mat4x4f orthoLH_NO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

/**
 * @brief Creates a left-handed orthographic matrix with a near plane of 0 and a far plane of 1.
 *
 * @param left The left plane.
 * @param right The right plane.
 * @param bottom The bottom plane.
 * @param top The top plane.
 *
 * @return The orthographic matrix.
*/
CORE_API_EXPORT mat4x4f orthoLH_ZO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

/**
 * @brief Creates a right-handed perspective matrix with a near plane of -1 and a far plane of 1.
 *
 * @param fovy The field of view in radians.
 * @param aspect The aspect ratio.
 * @param near The near plane.
 * @param far The far plane.
 *
 * @return The perspective matrix.
*/
CORE_API_EXPORT mat4x4f perspectiveRH_NO(radians fovy, f32 aspect, f32 near, f32 far);

/**
 * @brief Creates a right-handed perspective matrix with a near plane of 0 and a far plane of 1.
 *
 * @param fovy The field of view in radians.
 * @param aspect The aspect ratio.
 * @param near The near plane.
 * @param far The far plane.
 *
 * @return The perspective matrix.
*/
CORE_API_EXPORT mat4x4f perspectiveRH_ZO(radians fovy, f32 aspect, f32 near, f32 far);

/**
 * @brief Creates a left-handed perspective matrix with a near plane of -1 and a far plane of 1.
 *
 * @param fovy The field of view in radians.
 * @param aspect The aspect ratio.
 * @param near The near plane.
 * @param far The far plane.
 *
 * @return The perspective matrix.
*/
CORE_API_EXPORT mat4x4f perspectiveLH_NO(radians fovy, f32 aspect, f32 near, f32 far);

/**
 * @brief Creates a left-handed perspective matrix with a near plane of 0 and a far plane of 1.
 *
 * @param fovy The field of view in radians.
 * @param aspect The aspect ratio.
 * @param near The near plane.
 * @param far The far plane.
 *
 * @return The perspective matrix.
*/
CORE_API_EXPORT mat4x4f perspectiveLH_ZO(radians fovy, f32 aspect, f32 near, f32 far);

/**
 * @brief Creates a right-handed look-at matrix.
 *
 * @param eye The position of the camera.
 * @param center The position of the object to look at.
 * @param up The up vector.
 *
 * @return The look-at matrix.
*/
CORE_API_EXPORT mat4x4f lookAtRH(const vec3f& eye, const vec3f& center, const vec3f& up);

/**
 * @brief Creates a left-handed look-at matrix.
 *
 * @param eye The position of the camera.
 * @param center The position of the object to look at.
 * @param up The up vector.
 *
 * @return The look-at matrix.
*/
CORE_API_EXPORT mat4x4f lookAtLH(const vec3f& eye, const vec3f& center, const vec3f& up);

} // namespace core
