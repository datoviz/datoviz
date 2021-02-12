#version 450
#include "common.glsl"
// #include "colormaps.glsl"

layout(std140, binding = USER_BINDING) uniform Params
{
    int cmap;
    float scale;
} params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_cmap; // colormap texture
layout(binding = (USER_BINDING + 2)) uniform sampler2D tex;      // image

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main()
{
    // Fetch the value from the texture.
    float value = params.scale * texture(tex, in_uv).r;

    // Sampling from the color texture.
    out_color = texture(tex_cmap, vec2(value, (params.cmap + .5) / 256.0));

    // Or computing directly in the shader. Limited to a few colormaps. Not sure which is faster.
    // out_color = colormap(params.cmap, value);

    out_color.a = 1;
}
