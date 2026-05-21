#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 frontAccum;
layout(location = 1) out vec4 backAccum;
layout(location = 2) out vec4 depthPair;

void main()
{
    /* Init pass: reduce all transparent fragments to first nearest/farthest depth bounds. */
    backAccum = vec4(0.0);
    frontAccum = vec4(0.0);
    depthPair = vec4(gl_FragCoord.z, -gl_FragCoord.z, 0.0, 0.0);
}
