#version 450

#include "scene_material.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(applyDepthCue(fragColor.rgb, fragCue), fragColor.a);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
