#version 450

#include "stroke.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragCoord;
layout(location = 2) in float fragLength;
layout(location = 3) in float fragLineWidth;
layout(location = 4) in float fragHasPrev;
layout(location = 5) in float fragHasNext;
layout(location = 6) in float fragBevelDistance;

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
    if (fragCoord.x < 0.0)
    {
        if (!(fragHasPrev >= 0.5 && joinType == 2))
        {
            int capType = fragHasPrev < 0.5 ? int(round(material.params.x)) : joinType;
            if (fragHasPrev >= 0.5 && joinType == 0)
                capType = 5;
            alpha = dvz_stroke_cap_alpha(capType, -fragCoord.x, fragCoord.y, fragLineWidth);
            distance = dvz_stroke_cap_distance(capType, fragCoord.x, fragCoord.y, fragLineWidth);
        }
    }
    else if (fragCoord.x > fragLength)
    {
        if (!(fragHasNext >= 0.5 && joinType == 2))
        {
            int capType = fragHasNext < 0.5 ? int(round(material.params.y)) : joinType;
            if (fragHasNext >= 0.5 && joinType == 0)
                capType = 5;
            alpha = dvz_stroke_cap_alpha(
                capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
            distance = dvz_stroke_cap_distance(
                capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
        }
    }

    float bevelExtent = max(fragLineWidth, 0.0) * 0.5 + 2.0;
    bool bevelOverhang = joinType == 2 &&
                         ((fragCoord.x < bevelExtent && fragHasPrev >= 0.5) ||
                          (fragCoord.x > fragLength - bevelExtent && fragHasNext >= 0.5));
    if (bevelOverhang)
    {
        float bevelDistance = max(abs(distance), fragBevelDistance - 1.0);
        alpha = dvz_stroke_alpha(bevelDistance, fragLineWidth);
    }
    if (alpha <= 0.0)
        discard;
    outColor = fragColor;
}
