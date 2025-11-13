#version 450
layout(location=0) in vec2 in_pos;
layout(location=1) in uint in_id;

layout(location=0) out flat uint out_id;

void main() {
    gl_Position = vec4(in_pos, 0, 1);
    out_id = in_id;
}
