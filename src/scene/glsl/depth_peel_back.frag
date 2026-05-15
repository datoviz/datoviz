#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 frontAccum;
layout(location = 1) out vec4 backAccum;
layout(location = 2) out vec4 depthPair;

void main()
{
    float a = clamp(fragColor.a, 0.0, 1.0);
    frontAccum = vec4(0.0);
    backAccum = vec4(fragColor.rgb * a, a);
    depthPair = vec4(gl_FragCoord.z, 1.0 - gl_FragCoord.z, 0.0, 1.0);
}
