#version 450

#include "scene_material.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 1) uniform texture2D tex;
layout(set = 1, binding = 2) uniform sampler samp;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in float fragDepth;
layout(location = 4) in vec3 fragWorldPos;
layout(location = 5) in vec3 fragCameraPos;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 texel = texture(sampler2D(tex, samp), fragUV);
    vec4 base = texel * fragColor;
    vec4 shaded = evaluateSceneMaterial(base, fragNormal, fragWorldPos, fragCameraPos);
    vec3 cue = vec3(fragDepth, length(fragCameraPos - fragWorldPos), length(fragWorldPos));
    outColor = vec4(applyDepthCue(shaded.rgb, cue), shaded.a);
    if (outColor.a <= 0.0) {
        discard;
    }
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusionDepth(outColor, fragDepth);
#endif
}
