#version 450

#include "scene_material.glsl"
#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in float fragDepth;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

void main()
{
    vec3 n = normalize(fragNormal);
    if (!gl_FrontFacing)
        n = -n;
    vec4 shaded = evaluateSceneMaterial(fragColor, n, fragWorldPos, fragCameraPos);
    float a = clamp(shaded.a, 0.0, 1.0);
    vec3 cue = vec3(fragDepth, length(fragCameraPos - fragWorldPos), length(fragWorldPos));
    vec3 lit = applyDepthCue(shaded.rgb, cue);
    vec4 color = vec4(lit, a);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(color);
#endif
    a = clamp(color.a, 0.0, 1.0);
    outAccum = vec4(color.rgb * a, a);
    outWeight = -log(max(1.0 - a, 1e-4));
}
