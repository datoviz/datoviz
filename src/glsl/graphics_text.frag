#version 450
#include "common.glsl"

layout (binding = USER_BINDING) uniform TextParams {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  //
} params;

layout(binding = (USER_BINDING+1)) uniform sampler2D tex_sampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec2 glyph_size;
layout(location = 3) in float str_index;

layout(location = 0) out vec4 out_color;

#include "text_functions.glsl"

void main() {
    out_color = vec4(1,texture(tex_sampler, vec2(.5,.5)).x,0,1);
    // #include "text_frag.glsl"
    // out_color = texture(tex_sampler, vec2(.5,.5));
}
