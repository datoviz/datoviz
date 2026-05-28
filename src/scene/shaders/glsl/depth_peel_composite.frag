#version 450

layout(set = 0, binding = 0) uniform texture2D frontAccum;
layout(set = 0, binding = 1) uniform texture2D backAccum;
layout(set = 0, binding = 2) uniform sampler samp;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.xy);
    vec4 front = texelFetch(sampler2D(frontAccum, samp), uv, 0);
    vec4 back = texelFetch(sampler2D(backAccum, samp), uv, 0);
    float a = clamp(front.a + back.a * (1.0 - front.a), 0.0, 1.0);
    vec3 premul = front.rgb + back.rgb * (1.0 - front.a);
    outColor = a > 0.0 ? vec4(premul / a, a) : vec4(0.0);
}
