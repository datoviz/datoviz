#version 450

#include "common.glsl"

layout(set = 1, binding = 0) uniform SceneMaterial {
    vec4 lightDir;
    vec4 params;
    vec4 model;
    vec4 baseColorFactor;
    vec4 standardParams;
    vec4 emissiveRim;
    vec4 depthCue;
    vec4 depthCueColor;
    vec4 depthCueExtra;
} material;

layout(location = 0) in vec3 inPositionStart;
layout(location = 1) in vec3 inPositionEnd;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inLineWidth;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragCoord;
layout(location = 2) out float fragLength;
layout(location = 3) out float fragLineWidth;

vec2 clipToPixel(vec4 clip)
{
    vec2 ndc = clip.xy / max(abs(clip.w), 1e-6);
    return (ndc * 0.5 + 0.5) * viewport.rect.zw;
}

vec4 pixelToClip(vec2 pixel, float depth)
{
    vec2 ndc = pixel / max(viewport.rect.zw, vec2(1.0)) * 2.0 - 1.0;
    return vec4(ndc, depth, 1.0);
}

void main()
{
    vec4 startClip = transform(inPositionStart);
    vec4 endClip = transform(inPositionEnd);
    vec2 startPx = clipToPixel(startClip);
    vec2 endPx = clipToPixel(endClip);

    vec2 tangent = endPx - startPx;
    float lengthPx = length(tangent);
    if (lengthPx <= 1e-6)
        tangent = vec2(1.0, 0.0);
    else
        tangent /= lengthPx;
    vec2 normal = vec2(-tangent.y, tangent.x);

    float aa = 1.0;
    float halfWidth = max(inLineWidth, 0.0) * 0.5 + 1.5 * aa;
    int vertex = gl_VertexIndex & 3;
    vec2 pixel = startPx;
    float depth = startClip.z / max(abs(startClip.w), 1e-6);

    if (vertex == 0)
    {
        pixel = startPx - tangent * halfWidth + normal * halfWidth;
        fragCoord = vec2(-halfWidth, halfWidth);
    }
    else if (vertex == 1)
    {
        pixel = startPx - tangent * halfWidth - normal * halfWidth;
        fragCoord = vec2(-halfWidth, -halfWidth);
    }
    else if (vertex == 2)
    {
        pixel = endPx + tangent * halfWidth - normal * halfWidth;
        fragCoord = vec2(lengthPx + halfWidth, -halfWidth);
        depth = endClip.z / max(abs(endClip.w), 1e-6);
    }
    else
    {
        pixel = endPx + tangent * halfWidth + normal * halfWidth;
        fragCoord = vec2(lengthPx + halfWidth, halfWidth);
        depth = endClip.z / max(abs(endClip.w), 1e-6);
    }

    gl_Position = pixelToClip(pixel, depth);
    fragColor = inColor;
    fragLength = lengthPx;
    fragLineWidth = max(inLineWidth, 0.0);
}
