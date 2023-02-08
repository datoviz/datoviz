#version 450
#include "common.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
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
layout (location = 7) in uint transform_mode; // TODO

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_tex_coords;
layout (location = 2) out vec2 out_glyph_size;
layout (location = 3) out float out_str_index;


void main() {
    vec4 pos_tr = transform(pos, shift, transform_mode);

    mat4 ortho = get_ortho_matrix(viewport.size);
    mat4 ortho_inv = inverse(ortho);

    // Compute the rotation matrix for the glyphs.
    float cosa = cos(angle);
    float sina = sin(angle);
    mat2 rotation = mat2(cosa, -sina, sina, cosa);

    // Size of one glyph in NDC.
    float w = 2 * glyph_size.x;
    float h = 2 * glyph_size.y;

    // Which vertex within the triangle strip forming the rectangle.
    int i = gl_VertexIndex % 4;

    // Rectangle vertex displacement (one glyph = one rectangle = 6 vertices)
    float dx = int(i / 2.0);
    float dy = mod(i, 2.0);

    // Position of the glyph.
    vec2 origin = .5 * vec2(glyph.z * w, h) * (anchor - 1);
    vec2 p = origin + vec2(w * glyph.y, 0);

    // gl_Position = pos_tr;
    gl_Position = ortho_inv * pos_tr;
    gl_Position.xy += gl_Position.w * rotation * (p + vec2(dx * w, dy * h));  // bottom left of the glyph
    gl_Position = ortho * gl_Position;

    // Index in the texture
    float rows = params.grid_size.x;
    float cols = params.grid_size.y;

    float row = floor(glyph.x / cols);
    float col = mod(glyph.x, cols);

    // uv position in the texture for the glyph.
    vec2 uv = vec2(col, row);
    uv /= vec2(cols, rows);

    // Little margin to avoid edge effects between glyphs.
    float eps = .005;
    dx = eps + (1.0 - 2 * eps) * dx;
    dy = eps + (1.0 - 2 * eps) * dy;

    // Texture coordinates for the fragment shader.
    vec2 duv = vec2(dx / cols, dy /rows);

    // Output variables.
    out_tex_coords = uv + duv;

    // String index, used to discard between different strings.
    out_str_index = float(glyph.w);

    out_glyph_size = glyph_size;
    out_color = color;
}
