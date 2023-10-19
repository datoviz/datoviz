#version 450
#include "common.glsl"

#include "params_mesh.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
// layout(location = ) in vec2 uv;
// layout(location = ) in float alpha;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec4 out_color;
// layout(location = ) out vec2 out_uv;
// layout(location = ) out float out_clip;
// layout(location = ) out float out_alpha;

layout(constant_id = 0) const int MESH_MODE = 0;

void main()
{
    gl_Position = transform(pos);

    out_pos = ((mvp.model * vec4(pos, 1.0))).xyz;
    out_normal = ((transpose(inverse(mvp.model)) * vec4(normal, 1.0))).xyz;

    // out_uv = uv;
    // out_clip = dot(vec4(pos, 1.0), params.clip_coefs);
    // out_alpha = alpha;
    out_color = color;


    // NOTE: if uv.y is negative, we take uv.x and unpack the 3 first bytes and interpret them as
    // custom colors
    // if (uv.y < 0)
    //     out_color = unpack_color(uv).xyz;
}
