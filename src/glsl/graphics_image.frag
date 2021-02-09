#version 450
#include "common.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    vec4 tex_coefs; // blending coefficients for the textures
} params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_0;
layout(binding = (USER_BINDING + 2)) uniform sampler2D tex_1;
layout(binding = (USER_BINDING + 3)) uniform sampler2D tex_2;
layout(binding = (USER_BINDING + 4)) uniform sampler2D tex_3;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(0);
    if (params.tex_coefs.x > 0)
        out_color += params.tex_coefs.x * texture(tex_0, in_uv);
    if (params.tex_coefs.y > 0)
        out_color += params.tex_coefs.y * texture(tex_1, in_uv);
    if (params.tex_coefs.z > 0)
        out_color += params.tex_coefs.z * texture(tex_2, in_uv);
    if (params.tex_coefs.w > 0)
        out_color += params.tex_coefs.w * texture(tex_3, in_uv);
}
