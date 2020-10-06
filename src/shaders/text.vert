#version 450
#include "common.glsl"

layout (binding = 2) uniform TextParams {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  // (400, 200)
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 shift;
layout (location = 2) in vec4 color;
layout (location = 3) in vec2 glyph_size;
layout (location = 4) in vec2 anchor;
layout (location = 5) in float angle;
layout (location = 6) in uvec4 glyph;  // char, char_index, str_len, str_index
layout (location = 7) in uint transform_mode;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_tex_coords;
layout (location = 2) out vec2 out_glyph_size;
layout (location = 3) out float out_str_index;


void main() {
    vec4 pos_tr = transform_pos(pos, shift, transform_mode);

    mat4 ortho = get_ortho_matrix(mvp.viewport.zw);
    mat4 ortho_inv = inverse(ortho);

    // Compute the rotation matrix for the glyphs.
    float cosa = cos(angle);
    float sina = sin(angle);
    mat2 rotation = mat2(cosa, -sina, sina, cosa);

    #include "text_pos.glsl"

    out_glyph_size = glyph_size;
    out_color = get_color(color);
}
