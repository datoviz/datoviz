#version 450
#include "common.glsl"
#include "params_image.glsl"

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_size; // w, h, zoom

layout(location = 0) out vec4 out_color;

// from https://iquilezles.org/articles/distfunctions
float roundedBoxSDF(vec2 center, vec2 size, float radius)
{
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

void main()
{
    CLIP;

    vec2 size = in_size.xy;
    float zoom = in_size.z;
    vec2 xy = in_uv * size;
    float s = 1.0f;
    float radius = params.radius * zoom;
    float a = 1.0;

    if (radius > 0)
    {
        float d = roundedBoxSDF(xy - (size / 2.0f), size / 2.0f, radius);
        a = 1.0f - smoothstep(0.0f, s * 2.0f, d);
        if (a < 0.01)
            discard;
    }

    out_color = texture(tex, in_uv);
    out_color.a *= a;
}
