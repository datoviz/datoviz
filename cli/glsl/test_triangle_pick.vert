#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;
layout (location = 1) flat out vec4 out_index;

void main() {
    gl_Position = vec4(pos, 1.0);
    out_color = color;
    out_index = vec4(gl_VertexIndex, 0, 0, 0);
}
