#include "mesh_fragment_clip_fragment.glsl"
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
    applyFragmentClip();
    vec4 color = semanticColorToLinear(fragColor);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(color);
#endif
    outAccum = sceneWboitAccum(color);
    outWeight = sceneWboitWeight(color.a);
}
