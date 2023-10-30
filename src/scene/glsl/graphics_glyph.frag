#version 450
#include "common.glsl"
#include "params_glyph.glsl"


layout(binding = USER_BINDING + 1) uniform sampler2D tex;

// layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;
    out_color = vec4(1, 1, 1, 1);
    // out_color = texture(tex, in_uv);
}
