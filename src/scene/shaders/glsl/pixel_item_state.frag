#version 450

#include "scene_material.glsl"
#include "surface_depth.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(applyDepthCue(linearColor.rgb, fragCue), linearColor.a);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
