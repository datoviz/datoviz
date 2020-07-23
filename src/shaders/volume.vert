#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex_coords;

layout (location = 0) out vec2 out_tex_coords;

void main() {
    gl_Position = vec4(pos, 1.0);  // no transform in the vertex shader, in the fragment shader instead
    out_tex_coords = tex_coords;
}
