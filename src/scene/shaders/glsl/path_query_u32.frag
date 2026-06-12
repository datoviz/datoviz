#version 450

#include "stroke.glsl"

layout(location = 0) in vec2 fragCoord;
layout(location = 1) in float fragLength;
layout(location = 2) in float fragLineWidth;
layout(location = 3) in float fragHasPrev;
layout(location = 4) in float fragHasNext;
layout(location = 5) in vec2 fragBevelDistance;
layout(location = 6) flat in uint fragId;

layout(location = 0) out uint outId;

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
        if (fragHasPrev >= 0.5)
        {
            if (joinType == 1)
            {
                distance = length(fragCoord);
                alpha = dvz_stroke_alpha(distance, fragLineWidth);
            }
            else if (joinType == 0)
            {
                int capType = 5;
                alpha = dvz_stroke_cap_alpha(capType, -fragCoord.x, fragCoord.y, fragLineWidth);
                distance =
                    dvz_stroke_cap_distance(capType, fragCoord.x, fragCoord.y, fragLineWidth);
            }
        }
        else
        {
            int capType = int(round(material.params.x));
            alpha = dvz_stroke_cap_alpha(capType, -fragCoord.x, fragCoord.y, fragLineWidth);
            distance = dvz_stroke_cap_distance(capType, fragCoord.x, fragCoord.y, fragLineWidth);
        }
    }
    else if (fragCoord.x > fragLength)
    {
        if (fragHasNext >= 0.5)
        {
            if (joinType == 1)
            {
                distance = length(fragCoord - vec2(fragLength, 0.0));
                alpha = dvz_stroke_alpha(distance, fragLineWidth);
            }
            else if (joinType == 0)
            {
                int capType = 5;
                alpha = dvz_stroke_cap_alpha(
                    capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
                distance = dvz_stroke_cap_distance(
                    capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
            }
        }
        else
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
    if (fragCoord.x < 0.0 && fragHasPrev >= 0.5 && (joinType == 0 || joinType == 2))
    {
        float clipDistance = joinType == 2 ? bevelClip : miterClip;
        if (fragBevelDistance.x > abs(distance) + clipDistance)
            distance = fragBevelDistance.x - clipDistance;
        alpha = dvz_stroke_alpha(distance, fragLineWidth);
    }
    else if (fragCoord.x > fragLength && fragHasNext >= 0.5 && (joinType == 0 || joinType == 2))
    {
        float clipDistance = joinType == 2 ? bevelClip : miterClip;
        if (fragBevelDistance.y > abs(distance) + clipDistance)
            distance = fragBevelDistance.y - clipDistance;
        alpha = dvz_stroke_alpha(distance, fragLineWidth);
    }
    if (alpha <= 0.0)
        discard;
    outId = fragId;
}
