#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;

layout (std140, binding = 0) uniform UBO {
    vec4 color;
} ubo;


void main() {
    gl_Position = vec4(pos, 1.0);
    out_color.xyz = ubo.color.xyz;
    out_color.a = 1;
}
