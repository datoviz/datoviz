#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

#include "stroke.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragCoord;
layout(location = 2) in float fragLength;
layout(location = 3) in float fragLineWidth;

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
    float alpha = dvz_stroke_alpha(fragCoord.y, fragLineWidth);
    if (fragCoord.x < 0.0)
    {
        alpha = dvz_stroke_cap_alpha(
            int(round(material.params.x)), -fragCoord.x, fragCoord.y, fragLineWidth);
    }
    else if (fragCoord.x > fragLength)
    {
        alpha = dvz_stroke_cap_alpha(
            int(round(material.params.y)), fragCoord.x - fragLength, fragCoord.y,
            fragLineWidth);
    }

    if (alpha <= 0.0)
        discard;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
