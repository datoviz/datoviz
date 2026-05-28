#version 450

layout(location = 0) flat in uint fragId;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (dot(uv, uv) > 1.0)
        discard;
    outColor = vec4(
        float(fragId & 255u) / 255.0,
        float((fragId >> 8u) & 255u) / 255.0,
        float((fragId >> 16u) & 255u) / 255.0,
        float((fragId >> 24u) & 255u) / 255.0);
}
