#version 450
#include "common.glsl"
// #include "colormaps.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    int cmap;
} params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_cmap; // colormap texture
layout(binding = (USER_BINDING + 2)) uniform sampler3D tex; // 3D volume

layout (location = 0) in vec3 in_uvw;

layout (location = 0) out vec4 out_color;

void main() {
    float value = texture(tex, in_uvw).r;

    // Sampling from the color texture.
    out_color = texture(tex_cmap, vec2(value, params.cmap / 256.0));

    // Or computing directly in the shader. Limited to a few colormaps. Not sure which is faster.
    // out_color = colormap(params.cmap, value);

    // if (value < .1)
    //     out_color.a = 10 * value;
    // if (value < .01)
    //     discard;
}
