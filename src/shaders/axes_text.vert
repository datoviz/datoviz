#version 450
#include "common.glsl"

layout (binding = 2) uniform TextParams {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  // (400, 200)
    vec2 glyph_size;
    vec4 color;
} params;

layout (location = 0) in float tick;
layout (location = 1) in uint coord;
layout (location = 2) in vec2 anchor;
layout (location = 3) in uvec4 glyph;  // char, char_index, str_len, str_index

layout (location = 0) out vec2 out_tex_coords;
layout (location = 1) out float out_str_index;
layout (location = 2) out float out_coord;

void main() {

    out_coord = coord;
    vec3 pos = vec3(0);
    float margin_shift = 0;

    {
        float mt = mvp.margins[0];
        float mr = mvp.margins[1];
        float mb = mvp.margins[2];
        float ml = mvp.margins[3];
        float w = mvp.viewport.z;
        float h = mvp.viewport.w;
        float left = -1 + 2 * (ml - 10) / w;

        // x ticks
        float a, b;
        if (coord == 0) {
            // Take margins into account.
            a = 1 - (ml + mr) / w;
            b = (ml - mr) / w;
            pos.x = a * tick + b;
            pos.y = -1.0;
            margin_shift = -mb * 0.8;  // TODO: param (x tick label v padding)
        }
        // y ticks
        else if (coord == 1) {
            // Take margins into account.
            a = 1 - (mt + mb) / h;
            b = (mb - mt) / h;
            pos.y = a * tick + b;
            pos.x = left;
            margin_shift = -ml * .14;  // TODO: param (y tick label h padding)
        }
        pos.z = 0.0;
    }

    vec4 pos_tr = transform_pos(pos);
    mat4 ortho = get_ortho_matrix(mvp.viewport.zw);
    mat4 ortho_inv = inverse(ortho);
    mat2 rotation = mat2(1);

    // Fix ticks wrt pan/zoom.
    if (coord == 0) {
        pos_tr.y = -pos.y;  // NOTE: - because Vulkan inverses the y axis compared to visky/OpenGL convention
        pos_tr.y += 2 * margin_shift / mvp.viewport.w;
    }
    else if (coord == 1) {
        pos_tr.x = pos.x;
        pos_tr.x += 2 * margin_shift / mvp.viewport.z;
    }

    vec2 glyph_size = params.glyph_size;

    #include "text_pos.glsl"
}
