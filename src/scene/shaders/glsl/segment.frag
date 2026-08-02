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
