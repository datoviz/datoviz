#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in float fragDepth;
layout(location = 0) out vec4 outNormal;

void main()
{
    vec4 keepAlive = fragColor + vec4(fragWorldPos + fragCameraPos, fragDepth);
    vec3 n = normalize(fragNormal);
    float viewDistance = length(fragCameraPos - fragWorldPos);
    outNormal = vec4(n * 0.5 + 0.5, viewDistance) + keepAlive * 0.0;
}
