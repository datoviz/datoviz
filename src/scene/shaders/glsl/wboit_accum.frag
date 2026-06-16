#version 450

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
    float a = clamp(color.a, 0.0, 1.0);
    outAccum = vec4(color.rgb * a, a);
    outWeight = -log(max(1.0 - a, 1e-4));
}
