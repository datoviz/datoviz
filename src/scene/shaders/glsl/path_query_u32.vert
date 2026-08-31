#version 450

#include "common.glsl"
#include "stroke.glsl"

layout(set = 1, binding = 0) uniform SceneMaterial {
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
layout(location = 1) in vec3 inPositionStart;
layout(location = 2) in vec3 inPositionEnd;
layout(location = 3) in vec3 inPositionNext;
layout(location = 4) in uint inId;
layout(location = 5) in float inLineWidth;
layout(location = 6) in uint inPathFlags;
layout(location = 7) in float inPathDistance;

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

float computeU(vec2 p0, vec2 p1, vec2 p)
{
    vec2 v = p1 - p0;
    float l = max(length(v), 1e-6);
    return dot(p - p0, v) / l;
}

void main()
{
    bool sideNegative = (inPathFlags & SIDE_NEGATIVE) != 0u;
    bool endpointEnd = (inPathFlags & ENDPOINT_END) != 0u;
    bool hasPrev = (inPathFlags & HAS_PREV) != 0u;
    bool hasNext = (inPathFlags & HAS_NEXT) != 0u;
    float side = sideNegative ? -1.0 : 1.0;
    float strokeWidth = max(inLineWidth, 0.0);
    float lateralMarginPx = dvz_stroke_outer_half_width(strokeWidth);
    if (!hasPrev)
    {
        int startCap = int(round(material.params.x));
        lateralMarginPx = max(lateralMarginPx, dvz_stroke_cap_extension(startCap, strokeWidth));
        lateralMarginPx = max(lateralMarginPx, dvz_stroke_cap_half_width(startCap, strokeWidth));
    }
    if (!hasNext)
    {
        int endCap = int(round(material.params.y));
        lateralMarginPx = max(lateralMarginPx, dvz_stroke_cap_extension(endCap, strokeWidth));
        lateralMarginPx = max(lateralMarginPx, dvz_stroke_cap_half_width(endCap, strokeWidth));
    }

    vec4 p0Clip = transform(inPositionPrev);
    vec4 p1Clip = transform(inPositionStart);
    vec4 p2Clip = transform(inPositionEnd);
    vec4 p3Clip = transform(inPositionNext);

    // Picking has to agree with rendering, so this mirrors path.vert exactly:
    // clip before projecting, because deviceClipToTopLeftPixel() mirrors a
    // behind-camera vertex through the viewport centre rather than dropping it.
    vec4 startOriginal = p1Clip;
    vec4 endOriginal = p2Clip;
    if (!dvz_stroke_clip_to_view(p1Clip, p2Clip, lateralMarginPx, viewport.rect.zw))
    {
        gl_Position = vec4(2.0, 2.0, 1.0, 1.0);
        fragCoord = vec2(0.0);
        fragLength = 0.0;
        fragLineWidth = 0.0;
        fragHasPrev = 0.0;
        fragHasNext = 0.0;
        fragBevelDistance = vec2(0.0);
        fragJoinSplitDistance = 0.0;
        fragId = inId;
        return;
    }
    bool startClipped = p1Clip != startOriginal;
    bool endClipped = p2Clip != endOriginal;
    bool joinPrev = hasPrev && !startClipped;
    bool joinNext = hasNext && !endClipped;
    if (joinPrev)
        joinPrev = dvz_stroke_clip_near(p0Clip, p1Clip);
    if (joinNext)
        joinNext = dvz_stroke_clip_near(p3Clip, p2Clip);
    hasPrev = hasPrev || startClipped;
    hasNext = hasNext || endClipped;

    vec2 p0 = deviceClipToTopLeftPixel(p0Clip);
    vec2 p1 = deviceClipToTopLeftPixel(p1Clip);
    vec2 p2 = deviceClipToTopLeftPixel(p2Clip);
    vec2 p3 = deviceClipToTopLeftPixel(p3Clip);

    vec2 v0 = safeNormalize(p1 - p0, vec2(1.0, 0.0));
    vec2 v1 = safeNormalize(p2 - p1, v0);
    vec2 v2 = safeNormalize(p3 - p2, v1);
    if (!joinPrev)
        v0 = v1;
    if (!joinNext)
        v2 = v1;
    vec2 n0 = vec2(-v0.y, v0.x);
    vec2 n1 = vec2(-v1.y, v1.x);
    vec2 n2 = vec2(-v2.y, v2.x);

    float halfWidth = dvz_stroke_outer_half_width(strokeWidth);
    int joinType = int(round(material.params.z));
    float miterLimit = max(material.params.w, 1.0);
    float lengthPx = length(p2 - p1);
    vec2 miterStart = safeNormalize(n0 + n1, n1);
    vec2 miterEnd = safeNormalize(n1 + n2, n1);
    float denomStart = dot(miterStart, n1);
    float denomEnd = dot(miterEnd, n1);
    float lengthStart = denomStart > 1e-3 ? halfWidth / denomStart : halfWidth;
    float lengthEnd = denomEnd > 1e-3 ? halfWidth / denomEnd : halfWidth;
    float miterLengthLimit = max(miterLimit * (strokeWidth * 0.5) + 2.0, halfWidth);
    float nonMiterScale = mix(1.0, 2.5, smoothstep(12.0, 48.0, strokeWidth));
    float nonMiterLengthLimit = max(nonMiterScale * (strokeWidth * 0.5) + 2.0, halfWidth);
    float joinLengthLimit = joinType == 0 ? miterLengthLimit : nonMiterLengthLimit;
    lengthStart = min(lengthStart, joinLengthLimit);
    lengthEnd = min(lengthEnd, joinLengthLimit);

    int capType = endpointEnd ? int(round(material.params.y)) : int(round(material.params.x));
    vec2 pixel = p1;
    if (!endpointEnd)
    {
        if (!hasPrev)
        {
            float capExtension = dvz_stroke_cap_extension(capType, strokeWidth);
            float capHalfWidth = dvz_stroke_cap_half_width(capType, strokeWidth);
            pixel = p1 - capExtension * v1 + side * capHalfWidth * n1;
            fragCoord = vec2(-capExtension, side * capHalfWidth);
        }
        else
        {
            pixel = p1 + side * lengthStart * miterStart;
            fragCoord = vec2(computeU(p1, p2, pixel), side * halfWidth);
        }
    }
    else
    {
        if (!hasNext)
        {
            float capExtension = dvz_stroke_cap_extension(capType, strokeWidth);
            float capHalfWidth = dvz_stroke_cap_half_width(capType, strokeWidth);
            pixel = p2 + capExtension * v1 + side * capHalfWidth * n1;
            fragCoord = vec2(lengthPx + capExtension, side * capHalfWidth);
        }
        else
        {
            pixel = p2 + side * lengthEnd * miterEnd;
            fragCoord = vec2(computeU(p1, p2, pixel), side * halfWidth);
        }
    }

    vec4 referenceClip = endpointEnd ? p2Clip : p1Clip;
    gl_Position = topLeftPixelToDeviceClip(pixel, referenceClip);

    fragBevelDistance = vec2(-halfWidth);
    fragJoinSplitDistance = 0.0;
    float turnStart = v0.x * v1.y - v0.y * v1.x;
    float turnEnd = v1.x * v2.y - v1.y * v2.x;
    float d0 = turnStart > 0.0 ? -1.0 : +1.0;
    float d1 = turnEnd > 0.0 ? -1.0 : +1.0;
    float startDistance = lineDistance(p1 + d0 * n0 * halfWidth, p1 + d0 * n1 * halfWidth, pixel);
    float endDistance = lineDistance(p2 + d1 * n1 * halfWidth, p2 + d1 * n2 * halfWidth, pixel);
    fragBevelDistance.x = hasPrev ? (endpointEnd ? -startDistance : side * d0 * startDistance)
                                  : -startDistance;
    fragBevelDistance.y = hasNext ? (endpointEnd ? -side * d1 * endDistance : -endDistance)
                                  : -endDistance;
    fragLength = lengthPx;
    fragLineWidth = strokeWidth;
    fragHasPrev = hasPrev ? 1.0 : 0.0;
    fragHasNext = hasNext ? 1.0 : 0.0;
    fragId = inId;
}
