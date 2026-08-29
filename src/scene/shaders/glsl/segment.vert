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

layout(location = 0) in vec3 inPositionStart;
layout(location = 1) in vec3 inPositionEnd;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inLineWidth;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragCoord;
layout(location = 2) out float fragLength;
layout(location = 3) out float fragLineWidth;

const float CLIP_EPS = 1e-5;

bool clipSegmentPlane(inout vec4 startClip, inout vec4 endClip, float startDist, float endDist)
{
    if (startDist < 0.0 && endDist < 0.0)
        return false;
    if (startDist < 0.0 || endDist < 0.0)
    {
        float t = startDist / (startDist - endDist);
        vec4 clipped = mix(startClip, endClip, clamp(t, 0.0, 1.0));
        if (startDist < 0.0)
            startClip = clipped;
        else
            endClip = clipped;
    }
    return true;
}

bool clipSegmentToView(inout vec4 startClip, inout vec4 endClip)
{
    if (!clipSegmentPlane(startClip, endClip, startClip.w - CLIP_EPS, endClip.w - CLIP_EPS))
        return false;
    if (!clipSegmentPlane(startClip, endClip, startClip.x + startClip.w, endClip.x + endClip.w))
        return false;
    if (!clipSegmentPlane(startClip, endClip, startClip.w - startClip.x, endClip.w - endClip.x))
        return false;
    if (!clipSegmentPlane(startClip, endClip, startClip.y + startClip.w, endClip.y + endClip.w))
        return false;
    if (!clipSegmentPlane(startClip, endClip, startClip.w - startClip.y, endClip.w - endClip.y))
        return false;
    if (!clipSegmentPlane(startClip, endClip, startClip.z, endClip.z))
        return false;
    return clipSegmentPlane(startClip, endClip, startClip.w - startClip.z, endClip.w - endClip.z);
}

void main()
{
    vec4 startClip = transform(inPositionStart);
    vec4 endClip = transform(inPositionEnd);
    if (!clipSegmentToView(startClip, endClip))
    {
        gl_Position = vec4(2.0, 2.0, 1.0, 1.0);
        fragColor = vec4(0.0);
        fragCoord = vec2(0.0);
        fragLength = 0.0;
        fragLineWidth = 0.0;
        return;
    }

    vec2 startPx = deviceClipToTopLeftPixel(startClip);
    vec2 endPx = deviceClipToTopLeftPixel(endClip);

    vec2 tangent = endPx - startPx;
    float lengthPx = length(tangent);
    if (lengthPx <= 1e-6)
        tangent = vec2(1.0, 0.0);
    else
        tangent /= lengthPx;
    vec2 normal = vec2(-tangent.y, tangent.x);

    float strokeWidth = max(inLineWidth, 0.0);
    int startCap = int(round(material.params.x));
    int endCap = int(round(material.params.y));
    float startExtension = dvz_stroke_cap_extension(startCap, strokeWidth);
    float endExtension = dvz_stroke_cap_extension(endCap, strokeWidth);
    float startHalfWidth = dvz_stroke_cap_half_width(startCap, strokeWidth);
    float endHalfWidth = dvz_stroke_cap_half_width(endCap, strokeWidth);
    int vertex = gl_VertexIndex & 3;
    vec2 pixel = startPx;
    vec4 clip = startClip;

    if (vertex == 0)
    {
        pixel = startPx - tangent * startExtension + normal * startHalfWidth;
        fragCoord = vec2(-startExtension, startHalfWidth);
    }
    else if (vertex == 1)
    {
        pixel = startPx - tangent * startExtension - normal * startHalfWidth;
        fragCoord = vec2(-startExtension, -startHalfWidth);
    }
    else if (vertex == 2)
    {
        pixel = endPx + tangent * endExtension - normal * endHalfWidth;
        fragCoord = vec2(lengthPx + endExtension, -endHalfWidth);
        clip = endClip;
    }
    else
    {
        pixel = endPx + tangent * endExtension + normal * endHalfWidth;
        fragCoord = vec2(lengthPx + endExtension, endHalfWidth);
        clip = endClip;
    }

    gl_Position = topLeftPixelToDeviceClip(pixel, clip);
    fragColor = inColor;
    fragLength = lengthPx;
    fragLineWidth = strokeWidth;
}
