#version 450

#include "scene_material.glsl"

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
    outAccum = vec4(lit * a, a);
    outWeight = -log(max(1.0 - a, 1e-4));
}
