#version 450

layout(set = 3, binding = 0) uniform texture2D prevDepthMinMax;
layout(set = 3, binding = 1) uniform sampler samp;
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 frontAccum;
layout(location = 1) out vec4 backAccum;
layout(location = 2) out vec4 depthPair;

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.xy);
    vec2 prev = texelFetch(sampler2D(prevDepthMinMax, samp), uv, 0).rg;
    float maxDepth = prev.g > prev.r ? prev.g : 1.0;
    if (gl_FragCoord.z <= prev.r + 1e-5 || gl_FragCoord.z >= maxDepth - 1e-5)
        discard;

    float a = clamp(fragColor.a, 0.0, 1.0);
    frontAccum = vec4(0.0);
    backAccum = vec4(fragColor.rgb * a, a);
    depthPair = vec4(gl_FragCoord.z, maxDepth, 0.0, 1.0);
}
