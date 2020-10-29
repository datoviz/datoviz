#version 450
#include "common.glsl"
#include "colormaps.glsl"


layout (binding = 2) uniform ImageCmapParams {
    uint cmap;
    float scaling;
    float alpha;
    int origin; // 0 = upper left, 1 = upper right, 2 = lower right, 3 = lower left
    int axis; // 0 = x, 1 = y
} params;

layout (binding = 3) uniform sampler2D texture_sampler;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;


void main() {
    vec2 uv = in_uv.xy;

    // Switch origin and UV direction depending on the visual's params.
    if (params.origin == 1 || params.origin == 2) uv.x = 1 - uv.x;
    if (params.origin == 2 || params.origin == 3) uv.y = 1 - uv.y;
    if (params.axis == 1) uv.xy = uv.yx;

    float value = texture(texture_sampler, uv).r;
    value = clamp(params.scaling * value, 0, 1);

    // NOTE: the following line works but there are artifacts between pixels, might be due to
    // propagating rounding errors??
    // out_color = texture(color_texture, vec2(value, (params.cmap+.5) / 256.0));

    // Computing the colormap without fetching values from the color texture works better here.
    out_color = colormap(int(round(params.cmap)), value);
    out_color.a = params.alpha;
}
