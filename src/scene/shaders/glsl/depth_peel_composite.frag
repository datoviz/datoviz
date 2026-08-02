#version 450

layout(set = 0, binding = 0) uniform texture2D frontAccum;
layout(set = 0, binding = 1) uniform texture2D backAccum;
layout(set = 0, binding = 2) uniform sampler samp;
layout(location = 0) in vec2 localUv;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 extent = textureSize(sampler2D(frontAccum, samp), 0);
    ivec2 texel = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));
    vec4 front = texelFetch(sampler2D(frontAccum, samp), texel, 0);
    vec4 back = texelFetch(sampler2D(backAccum, samp), texel, 0);
    float a = clamp(front.a + back.a * (1.0 - front.a), 0.0, 1.0);
    vec3 premul = front.rgb + back.rgb * (1.0 - front.a);
    outColor = a > 0.0 ? vec4(premul / a, a) : vec4(0.0);
}
