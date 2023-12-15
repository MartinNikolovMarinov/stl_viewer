#include <math/core_projections.h>

namespace core {

// Ortho Projection

mat4x4f orthoRH_NO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    auto ret = mat4f::identity();

    ret[0][0] = 2.0f / (right - left);
    ret[1][1] = 2.0f / (top - bottom);
    ret[2][2] = -2.0f / (far - near);
    ret[3][0] = -(right + left) / (right - left);
    ret[3][1] = -(top + bottom) / (top - bottom);
    ret[3][2] = -(far + near) / (far - near);

    return ret;
}

mat4x4f orthoRH_ZO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    auto ret = mat4f::identity();

    ret[0][0] = 2.0f / (right - left);
    ret[1][1] = 2.0f / (top - bottom);
    ret[2][2] = - 1.0f / (far - near);
    ret[3][0] = -(right + left) / (right - left);
    ret[3][1] = -(top + bottom) / (top - bottom);
    ret[3][2] = -near / (far - near);

    return ret;
}

mat4x4f orthoLH_NO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    auto ret = mat4f::identity();

    ret[0][0] = 2.0f / (right - left);
    ret[1][1] = 2.0f / (top - bottom);
    ret[2][2] = 2.0f / (far - near);
    ret[3][0] = -(right + left) / (right - left);
    ret[3][1] = -(top + bottom) / (top - bottom);
    ret[3][2] = -(far + near) / (far - near);

    return ret;
}

mat4x4f orthoLH_ZO(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    auto ret = mat4f::identity();

    ret[0][0] = 2.0f / (right - left);
    ret[1][1] = 2.0f / (top - bottom);
    ret[2][2] = 1.0f / (far - near);
    ret[3][0] = -(right + left) / (right - left);
    ret[3][1] = -(top + bottom) / (top - bottom);
    ret[3][2] = -near / (far - near);

    return ret;
}

// Perspective Projection

mat4x4f perspectiveRH_NO(radians fovy, f32 aspect, f32 near, f32 far) {
    f32 tanHalfFovy = core::tan(f32(fovy) / 2.0f);

    mat4f ret (0);

    ret[0][0] = 1.0f / (aspect * tanHalfFovy);
    ret[1][1] = 1.0f / (tanHalfFovy);
    ret[2][2] = -(far + near) / (far - near);
    ret[2][3] = -1.0f;
    ret[3][2] = -(2.0f * far * near) / (far - near);

    return ret;
}

mat4x4f perspectiveRH_ZO(radians fovy, f32 aspect, f32 near, f32 far) {
    f32 tanHalfFovy = core::tan(f32(fovy) / 2.0f);

    mat4f ret (0);

    ret[0][0] = 1.0f / (aspect * tanHalfFovy);
    ret[1][1] = 1.0f / (tanHalfFovy);
    ret[2][2] = far / (near - far);
    ret[2][3] = -1.0f;
    ret[3][2] = -(far * near) / (far - near);

    return ret;
}

mat4x4f perspectiveLH_NO(radians fovy, f32 aspect, f32 near, f32 far) {
    f32 tanHalfFovy = core::tan(f32(fovy) / 2.0f);

    mat4f ret (0);

    ret[0][0] = 1.0f / (aspect * tanHalfFovy);
    ret[1][1] = 1.0f / (tanHalfFovy);
    ret[2][2] = far / (far - near);
    ret[2][3] = 1.0f;
    ret[3][2] = -(far * near) / (far - near);

    return ret;
}

mat4x4f perspectiveLH_ZO(radians fovy, f32 aspect, f32 near, f32 far) {
    f32 tanHalfFovy = core::tan(f32(fovy) / 2.0f);

    mat4f ret (0);

    ret[0][0] = 1.0f / (aspect * tanHalfFovy);
    ret[1][1] = 1.0f / (tanHalfFovy);
    ret[2][2] = (far + near) / (far - near);
    ret[2][3] = 1.0f;
    ret[3][2] = -(2.0f * far * near) / (far - near);

    return ret;
}

// Look At

mat4x4f lookAtRH(const vec3f& eye, const vec3f& center, const vec3f& up) {
    vec3f f = vnorm(center - eye);
    vec3f s = vnorm(vcross(f, up));
    vec3f u = vcross(s, f);

    f32 sdot = f32(vdot(s, eye));
    f32 udot = f32(vdot(u, eye));
    f32 fdot = f32(vdot(f, eye));

    mat4x4f ret (
        s.x(), u.x(), -f.x(), 0,
        s.y(), u.y(), -f.y(), 0,
        s.z(), u.z(), -f.z(), 0,
        -sdot, -udot,  fdot,  1
    );

    return ret;
}

mat4x4f lookAtLH(const vec3f& eye, const vec3f& center, const vec3f& up) {
    vec3f f = vnorm(center - eye);
    vec3f s = vnorm(vcross(up, f));
    vec3f u = vcross(f, s);

    f32 sdot = f32(vdot(s, eye));
    f32 udot = f32(vdot(u, eye));
    f32 fdot = f32(vdot(f, eye));

    mat4x4f ret (
        s.x(), u.x(), f.x(), 0,
        s.y(), u.y(), f.y(), 0,
        s.z(), u.z(), f.z(), 0,
        -sdot, -udot, -fdot, 1
    );

    return ret;
}


} // namespace core
