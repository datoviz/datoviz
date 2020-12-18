#version 450
#include "common.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  // (400, 200)
} params;

layout(binding = (USER_BINDING+1)) uniform sampler2D tex_sampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec2 glyph_size;
layout(location = 3) in float str_index;

layout(location = 0) out vec4 out_color;

#include "text_functions.glsl"

void main() {
    #include "text_frag.glsl"

    // out_color.g = tex_coords.y*2;
    // float alpha = get_alpha(tex_coords);
    // alpha = supersample(alpha);
    // out_color = color;
    // out_color.g = params.tex_size.x/700.;
}
