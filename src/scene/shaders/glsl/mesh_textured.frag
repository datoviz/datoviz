#version 450

#include "mesh_surface.glsl"
#include "surface_depth.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in float fragDepth;
layout(location = 4) in vec3 fragWorldPos;
layout(location = 5) in vec3 fragCameraPos;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = evaluateMeshSurface(
        fragColor, fragNormal, fragUV, fragDepth, fragWorldPos, fragCameraPos);
    if (!sceneAlphaValid(outColor.a, material.model)) {
        discard;
    }
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
