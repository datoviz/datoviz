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

layout(location = 0) in vec3 inPositionPrev;
layout(location = 1) in vec3 inPositionCurr;
layout(location = 2) in vec3 inPositionNext;
layout(location = 3) in vec4 inColor;
layout(location = 4) in float inLineWidth;
layout(location = 5) in uint inPathFlags;
layout(location = 6) in float inPathDistance;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragCoord;
layout(location = 2) out float fragLength;
layout(location = 3) out float fragLineWidth;
layout(location = 4) out float fragHasPrev;
layout(location = 5) out float fragHasNext;

const uint SIDE_NEGATIVE = 0x01u;
const uint ENDPOINT_END = 0x02u;
const uint HAS_PREV = 0x04u;
const uint HAS_NEXT = 0x08u;

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

vec2 safeNormalize(vec2 v, vec2 fallback)
{
    float n = length(v);
    if (n <= 1e-6)
        return fallback;
    return v / n;
}

float capExtension(int capType, float halfWidth)
{
    if (capType == 0 || capType == 5)
        return 0.0;
    return halfWidth;
}

void main()
{
    bool sideNegative = (inPathFlags & SIDE_NEGATIVE) != 0u;
    bool endpointEnd = (inPathFlags & ENDPOINT_END) != 0u;
    bool hasPrev = (inPathFlags & HAS_PREV) != 0u;
    bool hasNext = (inPathFlags & HAS_NEXT) != 0u;
    float side = sideNegative ? -1.0 : 1.0;

    vec4 prevClip = transform(inPositionPrev);
    vec4 currClip = transform(inPositionCurr);
    vec4 nextClip = transform(inPositionNext);
    vec2 prevPx = clipToPixel(prevClip);
    vec2 currPx = clipToPixel(currClip);
    vec2 nextPx = clipToPixel(nextClip);

    vec2 dirIn = safeNormalize(currPx - prevPx, vec2(1.0, 0.0));
    vec2 dirOut = safeNormalize(nextPx - currPx, dirIn);
    if (!hasPrev)
        dirIn = dirOut;
    if (!hasNext)
        dirOut = dirIn;
    vec2 normalIn = vec2(-dirIn.y, dirIn.x);
    vec2 normalOut = vec2(-dirOut.y, dirOut.x);

    float aa = 1.0;
    float strokeWidth = max(inLineWidth, 0.0);
    float halfWidth = strokeWidth * 0.5 + 1.5 * aa;
    int joinType = int(round(material.params.z));
    float miterLimit = max(material.params.w, 1.0);

    vec2 normal = endpointEnd ? normalIn : normalOut;
    if (hasPrev && hasNext && joinType == 0)
    {
        vec2 miter = safeNormalize(normalIn + normalOut, normal);
        float denom = max(abs(dot(miter, normalOut)), 1e-3);
        float miterScale = min(1.0 / denom, miterLimit);
        normal = miter * miterScale;
    }
    else if (hasPrev && hasNext && joinType == 1)
    {
        normal = safeNormalize(normalIn + normalOut, normal);
    }

    vec2 tangent = endpointEnd ? dirIn : dirOut;
    float lengthPx = endpointEnd ? length(currPx - prevPx) : length(nextPx - currPx);
    float along = endpointEnd ? lengthPx : 0.0;
    float tangentOffset = 0.0;
    int capType = endpointEnd ? int(round(material.params.y)) : int(round(material.params.x));
    if (!hasPrev && !endpointEnd)
        tangentOffset = -capExtension(capType, halfWidth);
    else if (!hasNext && endpointEnd)
        tangentOffset = capExtension(capType, halfWidth);
    else if (hasPrev && hasNext && joinType == 1)
        tangentOffset = endpointEnd ? halfWidth : -halfWidth;

    vec2 pixel = currPx + normal * side * halfWidth + tangent * tangentOffset;
    gl_Position = pixelToClip(pixel, currClip.z / max(abs(currClip.w), 1e-6));

    fragColor = inColor;
    fragCoord = vec2(along + tangentOffset, side * halfWidth);
    fragLength = lengthPx;
    fragLineWidth = strokeWidth;
    fragHasPrev = hasPrev ? 1.0 : 0.0;
    fragHasNext = hasNext ? 1.0 : 0.0;
}
