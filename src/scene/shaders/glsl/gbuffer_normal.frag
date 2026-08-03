#version 450

#include "common.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in float fragDepth;
layout(location = 0) out vec4 outDepth;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out float outCoverage;

void main()
{
    if (fragColor.a <= 0.0)
        discard;

    vec3 viewPos = (mvp.view * vec4(fragWorldPos, 1.0)).xyz;
    vec3 viewNormal = normalize(transpose(inverse(mat3(mvp.view))) * fragNormal);
    float coverage = 1.0;
    outDepth = vec4(max(-viewPos.z, 0.0), 0.0, 0.0, coverage);
    outNormal = vec4(viewNormal, 0.0);
    outCoverage = coverage;

    // Retain the complete producer interface until primitive capture gets its own vertex shader.
    outDepth.x += (length(fragCameraPos) + fragDepth) * 0.0;
}
