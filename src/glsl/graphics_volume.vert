#version 450
#include "common.glsl"

layout(std140, binding = USER_BINDING) uniform Params
{
    vec4 view_pos;
    int cmap;
}
params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 uvw;

layout(location = 0) out vec3 out_uvw;
layout(location = 1) out vec3 out_ray;

void main()
{
    gl_Position = transform(pos);
    out_uvw = uvw;

    out_ray = (mvp.model * vec4(pos, 1.0) - params.view_pos).xyz;
}
