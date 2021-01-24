#version 450
#include "common.glsl"

layout(std140, binding = USER_BINDING) uniform Params
{
    vec4 view_pos;
    int cmap;
}
params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_cmap; // colormap texture
layout(binding = (USER_BINDING + 2)) uniform sampler3D tex;      // 3D volume

layout(location = 0) in vec3 in_uvw;
layout(location = 1) in vec3 in_ray;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(normalize(in_ray), 1);
    // Fetch the value from the texture.
    // float value = texture(tex, in_uvw).r;

    // // Transfer function on the texture value.
    // if (sum(params.x_cmap) != 0)
    //     value = transfer(value, params.x_cmap, params.y_cmap);

    // // Transfer function on the texture value.
    // float alpha = 1.0;
    // if (sum(params.x_alpha) != 0)
    //     alpha = transfer(value, params.x_alpha, params.y_alpha);

    // // Sampling from the color texture.
    // out_color = texture(tex_cmap, vec2(value, (params.cmap + .5) / 256.0));

    // Or computing directly in the shader. Limited to a few colormaps. Not sure which is faster.
    // out_color = colormap(params.cmap, value);

    // out_color.a = alpha;
}