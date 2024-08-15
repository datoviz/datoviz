#version 450
#include "common.glsl"
#include "params_monoglyph.glsl"

float segment(float edge0, float edge1, float x)
{
    return step(edge0, x) * (1.0 - step(edge1, x));
}

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_bytes_012;
layout(location = 2) in vec3 in_bytes_345;

layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;

    vec2 uv = floor(gl_PointCoord.xy * 8.0);
    if (uv.x > 5.0)
        discard;
    if (uv.y > 7.0)
        discard;
    float index = floor((uv.y * 6.0 + uv.x) / 8.0);
    float offset = floor(mod(uv.y * 6.0 + uv.x, 8.0));
    float byte = segment(0.0, 1.0, index) * in_bytes_012.x   //
                 + segment(1.0, 2.0, index) * in_bytes_012.y //
                 + segment(2.0, 3.0, index) * in_bytes_012.z //
                 + segment(3.0, 4.0, index) * in_bytes_345.x //
                 + segment(4.0, 5.0, index) * in_bytes_345.y //
                 + segment(5.0, 6.0, index) * in_bytes_345.z;
    if (floor(mod(byte / (128.0 / pow(2.0, offset)), 2.0)) > 0.0)
    {
        out_color = in_color;
    }
    else
    {
        discard;
        // DEBUG
        // out_color = vec4(1, 1, 1, .25);
    }
}
