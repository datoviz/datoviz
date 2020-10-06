#version 450
#include "constants.glsl"
#include "common.glsl"

layout (location = 0) in vec3 P0;
layout (location = 1) in vec3 P1;
layout (location = 2) in vec4 shift;
layout (location = 3) in vec4 color;
layout (location = 4) in float linewidth;
layout (location = 5) in int cap0;
layout (location = 6) in int cap1;
layout (location = 7) in uint transform_mode;

layout (location = 0) out vec4  out_color;
layout (location = 1) out vec2  out_texcoord;
layout (location = 2) out float out_length;
layout (location = 3) out float out_linewidth;
layout (location = 4) out float out_cap;

void main (void)
{
    out_color = get_color(color);
    out_linewidth = linewidth;

    int index = gl_VertexIndex % 4;

    vec4 P0_ = transform_pos(P0, shift.xy, transform_mode);
    vec4 P1_ = transform_pos(P1, shift.zw, transform_mode);

    #include "segment.glsl"

}
