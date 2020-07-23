#version 450
#include "arrows.glsl"

layout (location = 0) in vec4  in_color;
layout (location = 1) in vec2  in_texcoord;
layout (location = 2) in float in_length;
layout (location = 3) in float in_body;
layout (location = 4) in float in_head;
layout (location = 5) in float in_linewidth;
layout (location = 6) in float in_arrow_type;

layout (location = 0) out vec4 out_color;

void main (void)
{
    float d = select_arrow(in_texcoord, in_body, in_head, in_linewidth, in_arrow_type);
    out_color = filled(d, in_linewidth, in_color);
}
