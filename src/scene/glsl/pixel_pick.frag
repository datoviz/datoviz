#version 450

layout(location = 0) flat in uint fragId;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(
        float(fragId & 255u) / 255.0,
        float((fragId >> 8u) & 255u) / 255.0,
        float((fragId >> 16u) & 255u) / 255.0,
        float((fragId >> 24u) & 255u) / 255.0);
}
