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
    bool filled = material.params.y > 0.5;
    bool stroke = material.params.z > 0.5 || material.params.w > 0.5;
    bool outline = material.params.w > 0.5;
    float strokeWidth = stroke ? max(2.0 * max(lineWidth, 1.0) / max(fragSize, 1.0), aa) : 0.0;
    float edgeMask = stroke ? 1.0 - smoothstep(strokeWidth - aa, strokeWidth + aa, -dist) : 0.0;
    float fillMask = (filled && !outline) ? 1.0 - edgeMask : 0.0;
    float strokeMask = stroke ? edgeMask : 0.0;
    float coverage = outer * max(fillMask, strokeMask);
    if (coverage <= 0.0)
        discard;

    vec4 edgeColor = material.baseColorFactor;
    vec4 color = mix(fragColor, edgeColor, strokeMask);
    outColor = vec4(color.rgb, color.a * coverage);
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
