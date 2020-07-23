#version 450

layout (location = 0) in vec4 in_color;
layout (location = 1) in float in_path_idx;

layout (location = 0) out vec4 out_color;

void main()
{
    if (fract(in_path_idx) > 0.) discard;
    out_color = in_color;
}
