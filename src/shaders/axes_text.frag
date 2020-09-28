#version 450
#include "common.glsl"

layout (binding = 2) uniform TextParams {
    ivec2 grid_size;  // (6, 16)
    ivec2 tex_size;  // (400, 200)
    vec2 glyph_size;
    vec4 color;
} params;

layout(binding = 3) uniform sampler2D tex_sampler;

layout (location = 0) in vec2 tex_coords;
layout (location = 1) in float str_index;
layout (location = 2) in float coord;

layout(location = 0) out vec4 out_color;

#include "text_functions.glsl"

void main() {
    vec2 uv = gl_FragCoord.xy - mvp.viewport.xy;

    float w = mvp.viewport.z;
    float h = mvp.viewport.w;

    float top = mvp.margins.x;
    float right = mvp.margins.y;
    float bottom = mvp.margins.z;
    float left = mvp.margins.w;

    // Discard top and right edges.
    if (uv.y < top * .5 || uv.x > w - right * .2)
        discard;

    // Bottom-left corner: discard along the diagonal
    if (uv.x < left && uv.y > h - bottom) {
        vec2 q0 = vec2(left, bottom);
        vec2 q1 = vec2(uv.x, h - uv.y);
        float s = (q0.x * q1.y - q0.y * q1.x);
        float eps = left * 5;
        if ((s > -eps && coord < .5) || (s < eps && coord > .5))
            discard;
    }

    vec4 color = params.color;

    #include "text_frag.glsl"
}
