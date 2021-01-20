#version 450
#include "common.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    mat4 lights_pos_0; // lights 0-3
    mat4 lights_params_0; // for each light, coefs for ambient, diffuse, specular
    vec4 view_pos;
    vec4 tex_coefs; // blending coefficients for the textures
    vec4 clip_coefs;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in float alpha;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;
layout (location = 3) out vec3 out_color;
layout (location = 4) out float out_clip;
layout (location = 5) out float out_alpha;

void main() {
    gl_Position = transform(pos);

    out_pos = ((mvp.model * vec4(pos, 1.0))).xyz;
    out_normal = ((transpose(inverse(mvp.model)) * vec4(normal, 1.0))).xyz;

    out_uv = uv;
    out_clip = dot(vec4(pos, 1.0), params.clip_coefs);
    out_alpha = alpha;
    out_color = vec3(0);

    // NOTE: if uv.y is negative, we take uv.x and unpack the 3 first bytes and interpret them as
    // custom colors
    if (uv.y < 0)
    {
        out_color.x = mod(uv.x, 256.0);
        out_color.y = mod(floor(uv.x / 256.0), 256.0);
        out_color.z = mod(floor(uv.x / 65536.0), 256.0);
        out_color /= 256.0;
    }
}
