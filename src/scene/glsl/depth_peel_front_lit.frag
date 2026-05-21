#version 450

#include "scene_material.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in float fragDepth;
layout(location = 0) out vec4 frontAccum;
layout(location = 1) out vec4 backAccum;
layout(location = 2) out vec4 depthPair;

vec4 shade()
{
    vec4 shaded = evaluateSceneMaterial(fragColor, fragNormal, fragWorldPos, fragCameraPos);
    vec3 cue = vec3(fragDepth, length(fragCameraPos - fragWorldPos), length(fragWorldPos));
    return vec4(applyDepthCue(shaded.rgb, cue), shaded.a);
}

void main()
{
    /* Init pass: reduce all transparent fragments to first nearest/farthest depth bounds. */
    frontAccum = vec4(0.0);
    backAccum = vec4(0.0);
    depthPair = vec4(gl_FragCoord.z, -gl_FragCoord.z, 0.0, 0.0);
}
