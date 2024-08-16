#version 450
#include "antialias.glsl"
#include "common.glsl"
#include "markers.glsl"
#include "params_image.glsl"

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_size; // w, h, zoom
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;

    vec2 size = in_size.xy;
    float zoom = in_size.z;

    vec2 P = in_uv - .5;

    float lw = params.edge_width;
    vec2 c = size + 2 * lw + antialias;

    float radius = params.radius * zoom;
    float d = marker_rounded_rect(P * c, size, radius);

    vec4 base_color = in_color;
    if (FILL == 0)
    {
        base_color = texture(tex, in_uv);
    }
    vec4 edge_color = params.edge_color;
    out_color = outline(d, lw, edge_color, base_color);
}
