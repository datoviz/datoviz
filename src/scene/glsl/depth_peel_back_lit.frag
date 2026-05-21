#version 450

#include "scene_material.glsl"

layout(set = 3, binding = 0) uniform texture2D prevDepthMinMax;
layout(set = 3, binding = 1) uniform sampler samp;
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
    ivec2 uv = ivec2(gl_FragCoord.xy);
    vec2 prev = texelFetch(sampler2D(prevDepthMinMax, samp), uv, 0).rg;
    float nearDepth = -prev.r;
    float farDepth = prev.g;
    float z = gl_FragCoord.z;
    float eps = 1e-3;

    if (farDepth < nearDepth - eps || z < nearDepth - eps || z > farDepth + eps)
        discard;

    vec4 c = shade();
    float a = clamp(c.a, 0.0, 1.0);
    vec4 color = vec4(c.rgb * a, a);
    frontAccum = vec4(0.0);
    backAccum = vec4(0.0);
    depthPair = vec4(-1.0, -1.0, 0.0, 0.0);

    if (z <= nearDepth + eps)
    {
        frontAccum = color;
    }
    else if (z >= farDepth - eps)
    {
        backAccum = color;
    }
    else
    {
        depthPair = vec4(-z, z, 0.0, 0.0);
    }
}
