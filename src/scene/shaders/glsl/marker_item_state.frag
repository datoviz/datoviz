#version 450

#include "scene_material.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragSize;
layout(location = 2) in float fragAngle;
layout(location = 3) flat in uint fragShape;
layout(location = 4) in float fragSpriteScale;

layout(location = 0) out vec4 outColor;

float sdTriangle(vec2 p)
{
    const float k = 1.7320508;
    p.x = abs(p.x) - 1.0;
    p.y = p.y + 1.0 / k;
    if (p.x + k * p.y > 0.0)
        p = vec2(p.x - k * p.y, -k * p.x - p.y) * 0.5;
    p.x -= clamp(p.x, -2.0, 0.0);
    return -length(p) * sign(p.y);
}

float sdBox(vec2 p, vec2 b)
{
    vec2 d = abs(p) - b;
    return length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0);
}

float sdRoundBox(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

float sdSegment(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float sdEllipse(vec2 p, vec2 r)
{
    return (length(p / r) - 1.0) * min(r.x, r.y);
}

float sdHeart(vec2 p)
{
    p = vec2(p.x, -p.y) * 0.82 + vec2(0.0, 0.18);
    p.x = abs(p.x);
    if (p.y + p.x > 1.0)
        return length(p - vec2(0.25, 0.75)) - 0.3535534;
    float h = max(p.x + p.y, 0.0);
    float d0 = dot(p - vec2(0.0, 1.0), p - vec2(0.0, 1.0));
    float d1 = dot(p - vec2(0.5 * h), p - vec2(0.5 * h));
    return sqrt(min(d0, d1)) * sign(p.x - p.y);
}

float markerDistance(vec2 p, uint shape)
{
    if (shape == 1u)
        return sdBox(p, vec2(1.0));
    if (shape == 2u)
        return sdTriangle(p);
    if (shape == 3u)
        return abs(p.x) + abs(p.y) - 1.0;
    if (shape == 4u)
        return min(sdBox(p, vec2(0.28, 1.0)), sdBox(p, vec2(1.0, 0.28)));
    if (shape == 5u)
        return abs(length(p) - 0.62) - 0.18;
    if (shape == 6u)
    {
        float ring = abs(length(p) - 0.48) - 0.04;
        float h = sdBox(p, vec2(1.0, 0.028));
        float v = sdBox(p, vec2(0.028, 1.0));
        float inner = length(p) - 0.24;
        float crosshair = max(min(h, v), -inner);
        return min(ring, crosshair);
    }
    if (shape == 7u)
    {
        float a = sdSegment(p, vec2(-0.9, 0.0), vec2(0.9, 0.0));
        float b = sdSegment(p, vec2(-0.45, -0.78), vec2(0.45, 0.78));
        float c = sdSegment(p, vec2(-0.45, 0.78), vec2(0.45, -0.78));
        return min(a, min(b, c)) - 0.09;
    }
    if (shape == 8u)
    {
        float a = sdSegment(p, vec2(-0.72, 0.58), vec2(0.0, -0.42));
        float b = sdSegment(p, vec2(0.0, -0.42), vec2(0.72, 0.58));
        return min(a, b) - 0.13;
    }
    if (shape == 9u)
    {
        float d = length(p - vec2(0.0, 0.46)) - 0.43;
        d = min(d, length(p - vec2(0.46, 0.0)) - 0.43);
        d = min(d, length(p - vec2(0.0, -0.46)) - 0.43);
        d = min(d, length(p - vec2(-0.46, 0.0)) - 0.43);
        return d;
    }
    if (shape == 10u)
    {
        float d = length(p - vec2(0.0, 0.36)) - 0.36;
        d = min(d, length(p - vec2(-0.34, -0.08)) - 0.36);
        d = min(d, length(p - vec2(0.34, -0.08)) - 0.36);
        d = min(d, sdBox(p - vec2(0.0, -0.58), vec2(0.16, 0.38)));
        return d;
    }
    if (shape == 11u)
    {
        float shaft = sdBox(p - vec2(-0.28, 0.0), vec2(0.58, 0.18));
        float head = sdTriangle(vec2(-p.y * 1.35, (p.x - 0.12) * 1.35));
        return min(shaft, head);
    }
    if (shape == 12u)
        return sdEllipse(p, vec2(0.98, 0.56));
    if (shape == 13u)
        return sdBox(p, vec2(1.0, 0.28));
    if (shape == 14u)
        return sdHeart(p);
    if (shape == 15u)
    {
        float left = abs(length(p - vec2(-0.42, 0.0)) - 0.35) - 0.12;
        float right = abs(length(p - vec2(0.42, 0.0)) - 0.35) - 0.12;
        return min(left, right);
    }
    if (shape == 16u)
    {
        float head = length(p - vec2(0.0, 0.28)) - 0.5;
        float tip = sdTriangle(vec2(p.x * 1.15, p.y * 1.15 + 0.3));
        float hole = -(length(p - vec2(0.0, 0.30)) - 0.18);
        return max(min(head, tip), hole);
    }
    if (shape == 17u)
    {
        float d = sdHeart(vec2(p.x, -p.y + 0.08));
        d = min(d, sdBox(p - vec2(0.0, -0.62), vec2(0.16, 0.34)));
        return d;
    }
    if (shape == 18u)
    {
        float tag = max(
            sdRoundBox(p - vec2(0.08, 0.0), vec2(0.86, 0.58), 0.12), p.x + p.y - 1.0);
        float hole = -(length(p - vec2(-0.54, 0.0)) - 0.12);
        return max(tag, hole);
    }
    if (shape == 19u)
        return sdBox(p, vec2(0.28, 1.0));
    if (shape == 20u)
        return sdRoundBox(p, vec2(0.96, 0.68), 0.18);
    return length(p) - 1.0;
}

void main()
{
    vec2 uv = (gl_PointCoord * 2.0 - 1.0) * max(fragSpriteScale, 1.0);
    float c = cos(fragAngle);
    float s = sin(fragAngle);
    vec2 p = mat2(c, -s, s, c) * uv;

    float dist = markerDistance(p, fragShape);
    float aa = max(fwidth(dist), 1e-6);
    float outer = 1.0 - smoothstep(-aa, aa, dist);
    if (outer <= 0.0)
        discard;

    float lineWidth = max(material.params.x, 0.0);
    int aspect = int(material.params.y + 0.5);
    bool filled = aspect == 0 || aspect == 2;
    bool stroke = aspect == 1 || aspect == 2;
    float strokeWidth = stroke ? max(2.0 * max(lineWidth, 1.0) / max(fragSize, 1.0), aa) : 0.0;
    float edgeMask = stroke ? 1.0 - smoothstep(strokeWidth - aa, strokeWidth + aa, -dist) : 0.0;
    float fillMask = filled ? 1.0 - edgeMask : 0.0;
    float strokeMask = stroke ? edgeMask : 0.0;
    float coverage = outer * max(fillMask, strokeMask);
    if (coverage <= 0.0)
        discard;

    vec4 edgeColor = material.baseColorFactor;
    vec4 color = filled ? mix(fragColor, edgeColor, strokeMask) : edgeColor;
    outColor = vec4(color.rgb, color.a * coverage);
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
