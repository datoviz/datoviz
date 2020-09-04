#version 450
#include "antialias.glsl"
#include "common.glsl"
#include "axes.glsl"

layout (location = 0) in vec2  in_texcoord;
layout (location = 1) in vec2  in_coord_level;
layout (location = 2) in float in_length;
layout (location = 3) in float in_linewidth;
layout (location = 4) in vec4 in_color;

layout (location = 0) out vec4 out_color;

void main (void)
{
    vec4 viewport = mvp.viewport;
    float linewidth = in_linewidth;

    float coord = in_coord_level.x;
    float level = in_coord_level.y;

    // float hlw = in_length + linewidth;
    float mb = 1;  // margin bottom increase
    float ml = 1;  // margin left increase
    if (coord == 0) mb += in_length;
    else if (coord == 1) ml += in_length;
    vec4 m = vec4(0, 0, mb, ml);
    _CLIP(MARGINS - m, VIEWPORT)

    vec4 color = in_color;

    // Caps.
    int pcap = params.cap;

    if (in_texcoord.x < 0.0) {
        out_color = cap (int(round(pcap)),
                         in_texcoord.x, in_texcoord.y,
                         linewidth, color);
    }
    else if (in_texcoord.x > in_length) {
        out_color = cap (int(round(pcap)),
                         in_texcoord.x - in_length, in_texcoord.y,
                         linewidth, color);
    }
    else {
        out_color = stroke(in_texcoord.y, linewidth, color);
    }
}
