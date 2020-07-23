#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 padding;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec2 out_uv;

void main() {
    gl_Position = vec4(pos, 1.0);  // no transform, only static colorbar (do not move with panzoom and camera)

    float pad_rel = 2 * padding.x / mvp.viewport.z;
    gl_Position.x = pos.x <= 0 ? pos.x + pad_rel : pos.x - pad_rel;

    pad_rel = 2 * padding.y / mvp.viewport.w;
    gl_Position.y = pos.y <= 0 ? pos.y + pad_rel : pos.y - pad_rel;

    out_uv = uv;
}
