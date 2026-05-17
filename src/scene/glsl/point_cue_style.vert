#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragCue;
layout(location = 2) out float fragSize;

void main()
{
    vec4 world = mvp.model * vec4(inPos, 1.0);
    vec4 tr = transform(inPos);
    gl_Position = tr;
    gl_PointSize = inSize;
    fragColor = inColor;
    fragCue = vec3(
        tr.z / max(abs(tr.w), 1e-6),
        length((mvp.view * world).xyz),
        length(world.xyz));
    fragSize = inSize;
}
