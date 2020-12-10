#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Push {
	vec3 push;
} push;

void main() {
    gl_Position = vec4(pos, 1.0);
    out_color = color;
    out_color.xyz = push.push;
}
