#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inSigma;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragOffsetPx;
layout(location = 2) out vec2 fragSigma;

const float CUTOFF_SIGMA = 3.0;

vec2 quad_corner(uint vertex_id)
{
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, -1.0),
        vec2(+1.0, -1.0),
        vec2(-1.0, +1.0),
        vec2(-1.0, +1.0),
        vec2(+1.0, -1.0),
        vec2(+1.0, +1.0));
    return corners[vertex_id % 6u];
}

void main()
{
    vec2 sigma = max(inSigma, vec2(0.000001));
    float extent = CUTOFF_SIGMA * max(sigma.x, sigma.y);
    vec2 corner = quad_corner(uint(gl_VertexIndex));
    vec4 center = transform(inPos);
    vec2 viewport_size = max(viewport.rect.zw, vec2(1.0));
    vec2 ndc_radius = 2.0 * vec2(extent / viewport_size.x, extent / viewport_size.y);

    gl_Position = vec4(center.xy + corner * ndc_radius * center.w, center.zw);
    fragColor = inColor;
    fragOffsetPx = corner * extent;
    fragSigma = sigma;
}
