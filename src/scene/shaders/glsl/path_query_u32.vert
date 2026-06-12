#version 450

#include "common.glsl"
#include "stroke.glsl"

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
layout(location = 3) in uint inId;
layout(location = 4) in float inLineWidth;
layout(location = 5) in uint inPathFlags;
layout(location = 6) in float inPathDistance;

layout(location = 0) out vec2 fragCoord;
layout(location = 1) out float fragLength;
layout(location = 2) out float fragLineWidth;
layout(location = 3) out float fragHasPrev;
layout(location = 4) out float fragHasNext;
layout(location = 5) out vec2 fragBevelDistance;
layout(location = 6) out float fragJoinSplitDistance;
layout(location = 7) flat out uint fragId;

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

float lineDistance(vec2 p0, vec2 p1, vec2 p)
{
    vec2 v = p1 - p0;
    float l2 = max(dot(v, v), 1e-6);
    float u = dot(p - p0, v) / l2;
    vec2 h = p0 + u * v;
    return length(p - h);
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

    float strokeWidth = max(inLineWidth, 0.0);
    float halfWidth = dvz_stroke_outer_half_width(strokeWidth);
    int joinType = int(round(material.params.z));
    float miterLimit = max(material.params.w, 1.0);

    vec2 tangent = endpointEnd ? dirIn : dirOut;
    vec2 segmentNormal = endpointEnd ? normalIn : normalOut;
    float lengthPx = endpointEnd ? length(currPx - prevPx) : length(nextPx - currPx);
    float along = endpointEnd ? lengthPx : 0.0;
    float tangentOffset = 0.0;

    vec2 normal = segmentNormal;
    vec2 miter = segmentNormal;
    float miterScale = 1.0;
    if (hasPrev && hasNext)
    {
        miter = safeNormalize(normalIn + normalOut, segmentNormal);
        float denom = dot(miter, segmentNormal);
        miterScale = denom > 1e-3 ? 1.0 / denom : 1.0;
        if (joinType == 1)
            normal = miter * miterScale;
        else if (joinType == 0 && miterScale <= miterLimit)
            normal = miter * miterScale;
    }

    int capType = endpointEnd ? int(round(material.params.y)) : int(round(material.params.x));
    float capHalfWidth = halfWidth;
    if (!hasPrev && !endpointEnd)
    {
        tangentOffset = -dvz_stroke_cap_extension(capType, strokeWidth);
        capHalfWidth = dvz_stroke_cap_half_width(capType, strokeWidth);
    }
    else if (!hasNext && endpointEnd)
    {
        tangentOffset = dvz_stroke_cap_extension(capType, strokeWidth);
        capHalfWidth = dvz_stroke_cap_half_width(capType, strokeWidth);
    }

    vec2 pixel = currPx + normal * side * capHalfWidth + tangent * tangentOffset;
    gl_Position = pixelToClip(pixel, currClip.z / max(abs(currClip.w), 1e-6));

    fragBevelDistance = vec2(-halfWidth);
    fragJoinSplitDistance = halfWidth;
    if (hasPrev && hasNext)
    {
        float turn = dirIn.x * dirOut.y - dirIn.y * dirOut.x;
        float outerSide = turn > 0.0 ? -1.0 : +1.0;
        vec2 bevelStart = currPx + outerSide * normalIn * halfWidth;
        vec2 bevelEnd = currPx + outerSide * normalOut * halfWidth;
        float bevelDistance = side * outerSide * lineDistance(bevelStart, bevelEnd, pixel);
        fragBevelDistance = vec2(bevelDistance);
        vec2 splitDir = safeNormalize(dirIn + dirOut, tangent);
        float ownerSide = endpointEnd ? -1.0 : +1.0;
        fragJoinSplitDistance = ownerSide * dot(pixel - currPx, splitDir);
    }
    if (hasPrev && hasNext && (joinType == 1 || joinType == 2))
    {
        vec2 segmentStartPx = endpointEnd ? prevPx : currPx;
        fragCoord =
            vec2(dot(pixel - segmentStartPx, tangent), dot(pixel - currPx, segmentNormal));
    }
    else
    {
        fragCoord = vec2(along + tangentOffset, side * capHalfWidth);
    }
    fragLength = lengthPx;
    fragLineWidth = strokeWidth;
    fragHasPrev = hasPrev ? 1.0 : 0.0;
    fragHasNext = hasNext ? 1.0 : 0.0;
    fragId = inId;
}
