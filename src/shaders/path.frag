#version 450
#include "antialias.glsl"
#include "common.glsl"

layout (binding = 2) uniform PathParams {
    float linewidth;
    float miter_limit;
    int capType;
    int roundJoin;
    int enableDepth;
} params;

layout (location = 0) in vec4 in_color;
layout (location = 1) in vec2 in_caps;
layout (location = 2) in float in_length;
layout (location = 3) in vec2 in_texcoord;
layout (location = 4) in vec2 in_bevel_distance;

layout (location = 0) out vec4 out_color;


void discard_depth(vec4 color) {
    if (params.enableDepth > 0 && color.a < .25)
        discard;
}


void main() {
    float distance = in_texcoord.y;
    vec4 color = in_color;
    float linewidth = params.linewidth;
    float miter_limit = params.miter_limit;

    if (in_caps.x < 0.0) {
        out_color = cap(params.capType, in_texcoord.x, in_texcoord.y, linewidth, color);
        discard_depth(out_color);
        return;
    }
    if (in_caps.y > in_length) {
        out_color = cap(params.capType, in_texcoord.x-in_length, in_texcoord.y, linewidth, color);
        discard_depth(out_color);
        return;
    }

    // Round join (instead of miter)
    if (params.roundJoin > 0) {
        if (in_texcoord.x < 0.0)          { distance = length(in_texcoord); }
        else if(in_texcoord.x > in_length) { distance = length(in_texcoord - vec2(in_length, 0.0)); }
    }

    // Miter limit
    float t = (miter_limit-1.0)*(linewidth/2.0) + antialias;

    if( (in_texcoord.x < 0.0) && (in_bevel_distance.x > (abs(distance) + t)) ) {
        distance = in_bevel_distance.x - t;
    }
    else if( (in_texcoord.x > in_length) && (in_bevel_distance.y > (abs(distance) + t)) ) {
        distance = in_bevel_distance.y - t;
    }
    out_color = stroke(distance, linewidth, color);
    discard_depth(out_color);
}
