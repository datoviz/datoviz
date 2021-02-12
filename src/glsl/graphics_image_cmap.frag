#version 450
#include "common.glsl"
// #include "colormaps.glsl"

layout(std140, binding = USER_BINDING) uniform Params
{
    vec2 vrange;
    int cmap;
} params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_cmap; // colormap texture
layout(binding = (USER_BINDING + 2)) uniform sampler2D tex;      // image

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main()
{
    // // Fetch the value from the texture.
    float value = texture(tex, in_uv).r; // we assume the texture format rescales in [0, 1]
    float v0 = params.vrange.x;
    float v1 = params.vrange.y;
    value = v0 + clamp(value, v0, v1) * (v1 - v0);

    // Sampling from the color texture.
    out_color = texture(tex_cmap, vec2(value, (params.cmap + .5) / 256.0));

    // Or computing directly in the shader. Limited to a few colormaps. Not sure which is faster.
    // out_color = colormap(params.cmap, value);

    out_color.a = 1;
}
