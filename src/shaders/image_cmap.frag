#version 450
#include "common.glsl"

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
    // value = int(round(value * 256) + .5) / 256.0;
    out_color = texture(color_texture, vec2(value, (params.cmap+.5) / 256.0));
    out_color.a = params.alpha;
}
