#version 450
#include "common.glsl"

layout (binding = 2) uniform VolPos {
    vec3 vol_pos;
} ubo;


layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 tex_coords;
layout (location = 2) in int plane;

layout (location = 0) out vec3 out_tex_coords;

void main() {
    vec3 vol_pos = ubo.vol_pos;
    vec3 pos_ = pos;
    out_tex_coords = tex_coords;

    switch (plane) {
        case 0:
            out_tex_coords.x = .5 + vol_pos.x;
            pos_.z = vol_pos.x;
            break;
        case 1:
            out_tex_coords.y = .5 + vol_pos.y;
            pos_.x = vol_pos.y;
            break;
        case 2:
            out_tex_coords.z = .5 + vol_pos.z;
            pos_.y = vol_pos.z;
            break;
        default:
            break;
    }

    gl_Position = transform_pos(pos_);
}
