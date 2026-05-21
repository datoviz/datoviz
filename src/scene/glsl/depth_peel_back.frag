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
    float nearDepth = prev.r;
    float farDepth = -prev.g;
    float z = gl_FragCoord.z;
    float eps = 1e-5;

    if (farDepth < nearDepth - eps || z < nearDepth - eps || z > farDepth + eps)
        discard;

    float a = clamp(fragColor.a, 0.0, 1.0);
    vec4 color = vec4(fragColor.rgb * a, a);
    frontAccum = vec4(0.0);
    backAccum = vec4(0.0);
    depthPair = vec4(1.0, 0.0, 0.0, 0.0);

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
        depthPair = vec4(z, -z, 0.0, 0.0);
    }
}
