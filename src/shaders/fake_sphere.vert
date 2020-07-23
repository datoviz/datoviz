#version 450
#include "common.glsl"

layout (binding = 2) uniform SphereParams {
    vec4 light_pos;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in float radius;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec3 out_pos;
layout (location = 2) out float out_radius;
layout (location = 3) out vec4 out_eye_pos;

void main() {
    out_radius = radius;
    out_color = get_color(color).xyz;

    mat4 MV = mvp.view * mvp.model;
    mat4 P = mvp.proj;

    // https://stackoverflow.com/a/8609184/1595060
    vec4 eyePos = MV * vec4(pos, 1);
    vec4 projCorner = P * vec4(0.5 * radius, 0.5 * radius, eyePos.z, eyePos.w);
    gl_PointSize = mvp.viewport.z * projCorner.x / projCorner.w;
    gl_Position = P * eyePos;
    out_pos = gl_Position.xyz;
    out_eye_pos = eyePos;
}
