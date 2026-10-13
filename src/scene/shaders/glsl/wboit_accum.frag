#version 450

#include "wboit.glsl"

#include "color.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

void main()
{
    vec4 color = semanticColorToLinear(fragColor);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(color);
#endif
    outAccum = sceneWboitAccum(color);
    outWeight = sceneWboitWeight(color.a);
}
