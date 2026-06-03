#version 450

layout(location = 0) flat in uint fragId;
layout(location = 1) in float fragSize;
layout(location = 0) out uint outId;

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
    outId = fragId;
}
