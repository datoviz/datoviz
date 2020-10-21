#version 450
#include "common.glsl"
#include "colormaps.glsl"


layout (binding = 2) uniform ImageCmapParams {
    uint cmap;
    float scaling;
    float alpha;
} params;

layout (binding = 3) uniform sampler2D texture_sampler;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;


void main() {
    float value = texture(texture_sampler, in_uv).r;
    value = clamp(params.scaling * value, 0, 1);

    // NOTE: the following line works but there are artifacts between pixels, might be due to
    // propagating rounding errors??
    // out_color = texture(color_texture, vec2(value, (params.cmap+.5) / 256.0));

    // Computing the colormap without fetching values from the color texture works better here.
    out_color = colormap(int(round(params.cmap)), value);
    out_color.a = params.alpha;
}
