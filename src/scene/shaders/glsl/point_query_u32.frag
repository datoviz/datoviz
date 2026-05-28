#version 450

layout(location = 0) flat in uint fragId;
layout(location = 0) out uint outId;

void main()
{
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (dot(uv, uv) > 1.0)
        discard;
    outId = fragId;
}
