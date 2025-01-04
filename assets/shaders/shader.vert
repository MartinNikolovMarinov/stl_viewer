#version 450

layout(location = 0) out vec3 fragColor;

const vec2 positions[6] = vec2[](
    // Left triangle:
    vec2(0.98, 0.98),
    vec2(-0.98, 0.98),
    vec2(-0.98, -0.98),

    // Right triangle:
    vec2(0.98, 0.98),
    vec2(-0.98, -0.98),
    vec2(0.98, -0.98)
);

const vec3 colors[6] = vec3[](
    // Left triangle:
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),

    // Right triangle:
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 0.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
