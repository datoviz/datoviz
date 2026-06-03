#version 450

layout(location = 0) flat in uint fragId;
layout(location = 1) in float fragSize;
layout(location = 0) out vec4 outColor;

float pointDiscDistance()
{
    float size = max(fragSize, 0.0);
    float spriteSize = max(size + 4.0, 1.0);
    vec2 p = gl_PointCoord.xy - vec2(0.5);
    return length(p * spriteSize) - 0.5 * size;
}

void main()
{
    if (pointDiscDistance() > 0.0)
        discard;
    outColor = vec4(
        float(fragId & 255u) / 255.0,
        float((fragId >> 8u) & 255u) / 255.0,
        float((fragId >> 16u) & 255u) / 255.0,
        float((fragId >> 24u) & 255u) / 255.0);
}
