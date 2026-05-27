#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out float fragDepth;
layout(location = 4) out vec3 fragWorldPos;
layout(location = 5) out vec3 fragCameraPos;

void main()
{
    vec4 world = mvp.model * vec4(inPos, 1.0);
    vec4 tr = transform(inPos);
    gl_Position = tr;
    fragColor = inColor;
    fragNormal = transpose(inverse(mat3(mvp.model))) * inNormal;
    fragUV = inUV;
    fragDepth = tr.z / max(abs(tr.w), 1e-6);
    fragWorldPos = world.xyz;
    fragCameraPos = (inverse(mvp.view) * vec4(0, 0, 0, 1)).xyz;
}
