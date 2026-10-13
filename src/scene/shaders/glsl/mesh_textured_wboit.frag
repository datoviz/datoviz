#version 450

#include "mesh_surface.glsl"
#include "wboit.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in float fragDepth;
layout(location = 4) in vec3 fragWorldPos;
layout(location = 5) in vec3 fragCameraPos;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

void main()
{
    vec4 color = evaluateMeshSurface(
        fragColor, fragNormal, fragUV, fragDepth, fragWorldPos, fragCameraPos);
    if (!sceneAlphaValid(color.a, material.model))
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(color);
#endif
    outAccum = sceneWboitAccum(color);
    outWeight = sceneWboitWeight(color.a);
}
