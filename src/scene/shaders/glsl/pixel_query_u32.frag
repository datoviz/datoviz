#version 450

layout(location = 0) flat in uint fragId;
layout(location = 0) out uint outId;

layout(location = 1) in float fragAlpha;

void main()
{
    if (!(fragAlpha > 0.0) || isinf(fragAlpha))
        discard;
    outId = fragId;
}
