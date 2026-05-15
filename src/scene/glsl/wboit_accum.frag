#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

void main()
{
    float a = clamp(fragColor.a, 0.0, 1.0);
    outAccum = vec4(fragColor.rgb * a, a);
    outWeight = -log(max(1.0 - a, 1e-4));
}
