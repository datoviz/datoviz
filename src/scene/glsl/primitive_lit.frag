#version 450

#include "scene_material.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in float fragDepth;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 shaded = evaluateSceneMaterial(fragColor, fragNormal, fragWorldPos, fragCameraPos);
    vec3 cue = vec3(fragDepth, length(fragCameraPos - fragWorldPos), length(fragWorldPos));
    outColor = vec4(applyDepthCue(shaded.rgb, cue), shaded.a);
}
