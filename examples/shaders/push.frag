#version 450

layout (location = 0) in vec4 in_color;
layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Push {
	vec2 push;
} push;

void main()
{
    out_color = in_color;
    out_color.xy = push.push;
}
