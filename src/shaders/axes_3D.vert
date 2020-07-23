#version 450
#include "constants.glsl"
#include "common.glsl"
#include "axes_3D.glsl"

layout (location = 0) in float in_tick;
layout (location = 1) in vec4 color;
layout (location = 2) in float linewidth;
layout (location = 3) in int cap0;
layout (location = 4) in int cap1;
layout (location = 5) in uint in_coord_side;

layout (location = 0) out vec4  out_color;
layout (location = 1) out vec2  out_texcoord;
layout (location = 2) out float out_length;
layout (location = 3) out float out_linewidth;
layout (location = 4) out float out_cap;

void main (void)
{
    out_color = get_color(color);
    vec3 P0 = vec3(0);
    vec3 P1 = vec3(0);

    bvec3 opposite;
    get_axes_3D_positions(in_tick, in_coord_side, P0, P1, opposite);

    vec4 P0_ = transform_pos(P0);
    vec4 P1_ = transform_pos(P1);

    int index = gl_VertexIndex % 4;
    out_linewidth = linewidth;

    #include "segment.glsl"

}
