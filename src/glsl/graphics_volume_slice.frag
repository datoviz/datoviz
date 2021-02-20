#version 450
#include "common.glsl"
// #include "colormaps.glsl"

layout(std140, binding = USER_BINDING) uniform Params
{
    vec4 x_cmap;
    vec4 y_cmap;
    vec4 x_alpha;
    vec4 y_alpha;
    int cmap;
    float scale;
}
params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_cmap; // colormap texture
layout(binding = (USER_BINDING + 2)) uniform sampler3D tex;      // 3D volume

layout(location = 0) in vec3 in_uvw;

layout(location = 0) out vec4 out_color;

void main()
{
    CLIP

    vec4 sampled = texture(tex, in_uvw);

    // Direct sample from RGB values
    if (params.cmap < 0) {
        out_color = sampled;
        return;
    }

    // Fetch the value from the texture.
    float value = params.scale * texture(tex, in_uvw).r;
    value = clamp(value, 0, .999999);

    // Transfer function on the texture value.
    if (sum(params.x_cmap) != 0) {
        value = transfer(value, params.x_cmap, params.y_cmap);
        value = clamp(value, 0, .999999);
    }

    // Transfer function on the alpha value.
    float alpha = 1.0;
    if (sum(params.x_alpha) != 0) {
        alpha = transfer(value, params.x_alpha, params.y_alpha);
        alpha = clamp(alpha, 0, 1);
    }

    // Sampling from the color texture.
    out_color = texture(tex_cmap, vec2(value, (params.cmap + .5) / 256.0));

    // Or computing directly in the shader. Limited to a few colormaps. Not sure which is faster.
    // out_color = colormap(params.cmap, value);

    out_color.a = alpha;
    if (alpha < .01) discard;
}
