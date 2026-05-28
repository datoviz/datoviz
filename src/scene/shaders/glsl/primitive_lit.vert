#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragCameraPos;
layout(location = 4) out float fragDepth;

void main()
{
    vec4 world = mvp.model * vec4(inPos, 1.0);
    vec4 tr = transform(inPos);
    gl_Position = tr;
    fragColor = inColor;
    fragWorldPos = world.xyz;
    fragCameraPos = (inverse(mvp.view) * vec4(0, 0, 0, 1)).xyz;
    fragNormal = transpose(inverse(mat3(mvp.model))) * inNormal;
    fragDepth = tr.z / max(abs(tr.w), 1e-6);
}
