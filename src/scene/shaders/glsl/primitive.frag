#version 450

#include "color.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = semanticColorToLinear(fragColor);
    if (outColor.a <= 0.0) {
        discard;
    }
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
