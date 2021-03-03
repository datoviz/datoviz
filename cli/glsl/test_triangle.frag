#version 450

layout (location = 0) in vec4 in_color;
layout (location = 1) flat in uvec4 in_index;

layout (location = 0) out vec4 out_color;
layout (location = 1) out uvec4 out_pick;

void main()
{
    out_color = in_color;

    out_pick = in_index;
}
