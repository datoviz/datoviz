#version 450
#include "antialias.glsl"
#include "common.glsl"
#include "markers.glsl"


layout(binding = USER_BINDING) uniform MarkersParams
{
    vec4 edge_color;
    float edge_width;
}
params;

layout(location = 0) in vec4 color;
layout(location = 1) in float size;
layout(location = 2) in float angle;

layout(location = 0) out vec4 out_color;


void main()
{
    CLIP;

    float a = angle;
    float c = cos(angle);
    float s = sin(angle);

    // Pixel coordinates in [-0.5, -0.5, +0.5, +0.5].
    vec2 P = gl_PointCoord.xy - vec2(0.5, 0.5);

    // NOTE: rescale according to the rotation, to keep the marker size fixed while
    // the underlying square marker container is bigger to account for the rotation
    // of the marker.
    P *= (abs(c) + abs(s));

    // Marker rotation.
    mat2 rot = mat2(c, s, -s, c);
    P = rot * P;

    // Marker SDF.
    float distance = marker_heart(P * (size + 2 * params.edge_width + antialias), size);

    // Outline/filled.
    if (params.edge_width > 0)
    {
        out_color = outline(distance, params.edge_width, params.edge_color, color);
    }
    else
    {
        out_color = filled(distance, params.edge_width, color);
    }

    if (out_color.a < .01)
    {
        discard;
    }

    // DEBUG
    // out_color.a = max(out_color.a, .5);
    // out_color.b = .75;
}
