#version 450

#include "color.glsl"
#include "surface_depth.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

#include "stroke.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragCoord;
layout(location = 2) in float fragLength;
layout(location = 3) in float fragLineWidth;
layout(location = 4) in float fragHasPrev;
layout(location = 5) in float fragHasNext;
layout(location = 6) in vec2 fragBevelDistance;
layout(location = 7) in float fragJoinSplitDistance;

layout(location = 0) out vec4 outColor;

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

void main()
{
    float distance = fragCoord.y;
    float alpha = dvz_stroke_alpha(distance, fragLineWidth);
    int joinType = int(round(material.params.z));
    float aa = 1.0;
    if (fragCoord.x < 0.0)
    {
        if (fragHasPrev >= 0.5 && joinType == 1)
        {
            distance = length(fragCoord);
            alpha = dvz_stroke_alpha(distance, fragLineWidth);
        }
        else if (fragHasPrev < 0.5)
        {
            int capType = int(round(material.params.x));
            alpha = dvz_stroke_cap_alpha(capType, -fragCoord.x, fragCoord.y, fragLineWidth);
            distance = dvz_stroke_cap_distance(capType, fragCoord.x, fragCoord.y, fragLineWidth);
        }
    }
    else if (fragCoord.x > fragLength)
    {
        if (fragHasNext >= 0.5 && joinType == 1)
        {
            distance = length(fragCoord - vec2(fragLength, 0.0));
            alpha = dvz_stroke_alpha(distance, fragLineWidth);
        }
        else if (fragHasNext < 0.5)
        {
            int capType = int(round(material.params.y));
            alpha = dvz_stroke_cap_alpha(
                capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
            distance = dvz_stroke_cap_distance(
                capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
        }
    }

    float miterLimit = max(material.params.w, 1.0);
    float miterClip = (miterLimit - 1.0) * (fragLineWidth * 0.5) + aa;
    float bevelClip = aa;
    bool startBevel = joinType == 2 && fragCoord.x < 0.0 && fragHasPrev >= 0.5;
    bool endBevel = joinType == 2 && fragCoord.x > fragLength && fragHasNext >= 0.5;
    if (startBevel)
    {
        if (fragBevelDistance.x > abs(distance) + bevelClip)
            distance = fragBevelDistance.x - bevelClip;
        alpha = dvz_stroke_alpha(distance, fragLineWidth);
    }
    else if (endBevel)
    {
        if (fragBevelDistance.y > abs(distance) + bevelClip)
            distance = fragBevelDistance.y - bevelClip;
        alpha = dvz_stroke_alpha(distance, fragLineWidth);
    }
    else if (fragCoord.x < 0.0 && fragHasPrev >= 0.5 && joinType == 0)
    {
        if (fragBevelDistance.x > abs(distance) + miterClip)
            distance = fragBevelDistance.x - miterClip;
        alpha = dvz_stroke_alpha(distance, fragLineWidth);
    }
    else if (fragCoord.x > fragLength && fragHasNext >= 0.5 && joinType == 0)
    {
        if (fragBevelDistance.y > abs(distance) + miterClip)
            distance = fragBevelDistance.y - miterClip;
        alpha = dvz_stroke_alpha(distance, fragLineWidth);
    }
    if (alpha <= 0.0)
        discard;
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(linearColor.rgb, linearColor.a * alpha);
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
