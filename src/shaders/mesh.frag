#version 450
#include "common.glsl"
#include "lighting.glsl"
#include "wire_frag.glsl"

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

layout (binding = 3) uniform sampler2D texture_sampler;

layout (location = 0) in vec3 in_pos_world;
layout (location = 1) in vec3 in_normal_world;
layout (location = 2) in vec4 in_color;
layout (location = 3) in vec2 in_tex_coords;
layout (location = 4) in vec3 in_triangle_coords;

layout (location = 0) out vec4 out_color;


void main() {
    out_color = in_color;

    // Fetch texture if there is one.
    if (params.mode_color == MESH_COLOR_UV) {
        out_color = texture(texture_sampler, in_tex_coords);
    }

    // Apply shading if required.
    if (params.mode_shading > 0) {
        vec3 view_pos = mvp.view[3].xyz;
        vec3 light_pos = params.light_pos.xyz;
        out_color = lighting(out_color, in_pos_world, in_normal_world, view_pos, light_pos, params.light_coefs.xyz);
    }

    // Wire.
    if (params.wire_linewidth > 0) {
        out_color = add_wire(out_color, in_triangle_coords, params.wire_linewidth);
    }
}
