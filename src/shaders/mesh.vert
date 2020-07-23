#version 450
#include "common.glsl"
#include "wire_vert.glsl"

#define UINT16_MAX 65535

#define MESH_COLOR_RGBA     1
#define MESH_COLOR_UV       2

layout (binding = 2) uniform MeshParams {
    vec4 light_pos;
    vec4 light_coefs;
    ivec2 tex_size;
    int mode_color; // how to interpret the u16vec2 color attribute
    int mode_shading;
    float wire_linewidth;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in uvec4 color;  // four uint8, their interpretation depend on params.color_mode

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec4 out_color;
layout (location = 3) out vec2 out_tex_coords;
layout (location = 4) out vec3 out_triangle_coords;

void main() {
    gl_Position = transform_pos(pos);
    out_color = vec4(0, 0, 0, 1);
    out_tex_coords = vec2(0, 0);
    switch (params.mode_color) {
        case MESH_COLOR_RGBA:
            out_color = color / 255.0;
            break;
        case MESH_COLOR_UV:
            out_tex_coords = vec2(color.x * 256.0 + color.y, color.z * 256.0 + color.w) / UINT16_MAX;
            break;
        default:
            break;
    }

    out_pos = (mvp.model * vec4(pos, 1.0)).xyz;
    out_normal = (transpose(inverse(mvp.model)) * vec4(normal, 1.0)).xyz;
    out_triangle_coords = triangle_coords();
}
