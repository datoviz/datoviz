#version 450
#include "common.glsl"
#include "colormaps.glsl"


layout (location = 0) in vec2 pos;
layout (location = 1) in float depth;
layout (location = 2) in float cmap_val;
layout (location = 3) in float alpha;
layout (location = 4) in float size;

layout (location = 0) out vec4 out_color;

layout(std140, binding = USER_BINDING) uniform Params
{
    float alpha_max;
    float size_max;
    int cmap_id;
} params;



void main() {
    gl_Position = transform(vec3(pos, depth));

    // Marker color.
    out_color = colormap(params.cmap_id, cmap_val);
    out_color.a = alpha * params.alpha_max;

    // Marker size.
    float out_size = size * params.size_max;
    gl_PointSize = out_size;
}
