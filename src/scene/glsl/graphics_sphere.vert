#version 450
#include "common.glsl"

layout(binding = 2) uniform SphereParams
{
    vec4 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in float radius;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec3 out_pos;
layout(location = 2) out float out_radius;
layout(location = 3) out vec4 out_eye_pos;

void main()
{
    out_pos = pos;
    out_radius = radius;
    out_color = color;

    // Calculate the eye-space position
    vec4 world_pos = mvp.model * vec4(pos, 1.0);
    vec4 eye_pos = mvp.view * world_pos;
    out_eye_pos = eye_pos;

    // Project the position to clip space using the transform function
    gl_Position = transform(pos);

    // Set the point size to the diameter of the sphere in pixels
    float distance_to_camera = length(eye_pos.xyz);
    gl_PointSize = (2.0 * radius) / distance_to_camera;
}
