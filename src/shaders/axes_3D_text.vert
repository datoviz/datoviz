#version 450
#include "common.glsl"
#include "axes_3D.glsl"

layout (binding = 2) uniform TextParams {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  // (400, 200)
    vec2 glyph_size;
    vec4 color;
} params;

layout (location = 0) in float tick;
layout (location = 1) in uint coord_side; // H: 0x,1y,2z,(3). V: 4x, 5y, 6z, (7)
layout (location = 2) in uvec4 glyph;  // char, char_index, str_len, str_index

layout (location = 0) out vec2 out_tex_coords;
layout (location = 1) out float out_str_index;

#define MARGIN 1.075

void main() {

    vec3 P0 = vec3(0);
    vec3 P1 = vec3(0);
    bvec3 opposite;
    get_axes_3D_positions(tick, MARGIN, coord_side, P0, P1, opposite);
    ivec3 opp = 1 - 2 * ivec3(opposite);

    vec2 anchor = vec2(0);

    vec3 pos = vec3(0);
    vec4 pos_tr = vec4(0);
    float m = 2 * opp[coord_side % 4];
    switch (coord_side)
    {
    case 0:
        pos = opposite.z ? P1 : P0;
        pos_tr = transform_pos(pos);
        break;

    case 1:
        pos = opposite.x ? P0 : P1;
        pos_tr = transform_pos(pos);
        break;

    case 5:
        pos = opposite.z ? P0 : P1;
        pos_tr = transform_pos(pos);
        break;

    default:
        break;
    }



    mat4 ortho = get_ortho_matrix(mvp.viewport.zw);
    mat4 ortho_inv = inverse(ortho);
    mat2 rotation = mat2(1);
    vec2 glyph_size = params.glyph_size;

    #include "text_pos.glsl"
}
