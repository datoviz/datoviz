#version 450
#include "common.glsl"

layout (binding = 2) uniform TextParams {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  // (400, 200)
    vec2 glyph_size;
    vec4 color;
} params;

layout(binding = 3) uniform sampler2D tex_sampler;

layout (location = 0) in vec2 tex_coords;
layout (location = 1) in float str_index;

layout(location = 0) out vec4 out_color;

#include "text_functions.glsl"

void main() {
    vec2 uv = gl_FragCoord.xy - mvp.viewport.xy;
    vec4 color = params.color;

    #include "text_frag.glsl"
}
