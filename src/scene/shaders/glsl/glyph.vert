#version 450

#include "common.glsl"

#define DVZ_MVP_FLAGS_ISOTROPIC_LOCAL 1u

layout(location = 0) in vec3 inAnchor;
layout(location = 1) in vec4 inBounds;
layout(location = 2) in vec4 inUVBounds;
layout(location = 3) in vec4 inColor;
layout(location = 4) in float inAngle;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

vec2 corner(uint vertex_index)
{
    uint k = vertex_index % 6u;
    if (k == 0u) return vec2(0.0, 0.0);
    if (k == 1u) return vec2(0.0, 1.0);
    if (k == 2u) return vec2(1.0, 0.0);
    if (k == 3u) return vec2(1.0, 0.0);
    if (k == 4u) return vec2(0.0, 1.0);
    return vec2(1.0, 1.0);
}

vec2 local_pixel_delta(vec2 local)
{
    float c = cos(inAngle);
    float s = sin(inAngle);
    vec2 rotated = vec2(c * local.x - s * local.y, s * local.x + c * local.y);
    vec2 delta = vec2(
        viewport.rect.z > 0.0 ? 2.0 * rotated.x / viewport.rect.z : 0.0,
        viewport.rect.w > 0.0 ? -2.0 * rotated.y / viewport.rect.w : 0.0);

    if ((mvp.flags & DVZ_MVP_FLAGS_ISOTROPIC_LOCAL) != 0u)
    {
        mat4 tr = mvp.proj * mvp.view * mvp.model;
        float sx = length((tr * vec4(1.0, 0.0, 0.0, 0.0)).xy);
        float sy = length((tr * vec4(0.0, 1.0, 0.0, 0.0)).xy);
        float iso = sqrt(max(sx * sy, 1e-12));
        delta.x *= sx > 1e-6 ? iso / sx : 1.0;
        delta.y *= sy > 1e-6 ? iso / sy : 1.0;
    }
    return delta;
}

void main()
{
    vec2 k = corner(uint(gl_VertexIndex));
    vec2 local = mix(inBounds.xy, inBounds.zw, k);
    vec2 uv = mix(inUVBounds.xy, inUVBounds.zw, k);
    gl_Position = transform(inAnchor + vec3(local_pixel_delta(local), 0.0));
    fragUV = uv;
    fragColor = inColor;
}
