#version 450

#include "scene_material.glsl"
#include "wboit.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 2) in float fragSize;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

#include "point_disc.glsl"

#include "point_style_surface.glsl"

void main()
{
    vec4 outColor;
    vec4 color = pointStyleSurface();
    outColor = vec4(applyDepthCue(color.rgb, fragCue), color.a);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
    if (outColor.a <= 0.0)
        discard;
    outAccum = sceneWboitAccum(outColor);
    outWeight = sceneWboitWeight(outColor.a);
}
