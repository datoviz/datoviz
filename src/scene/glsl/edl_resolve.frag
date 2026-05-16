#version 450

layout(set = 0, binding = 0) uniform texture2D colorTex;
layout(set = 0, binding = 1) uniform texture2D depthTex;
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform EdlParams {
    vec4 params;
} edl;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    ivec2 extent = textureSize(sampler2D(colorTex, samp), 0);
    if (p.x < 0 || p.y < 0 || p.x >= extent.x || p.y >= extent.y)
        discard;

    vec4 color = texelFetch(sampler2D(colorTex, samp), p, 0);
    float center = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    if (edl.params.w <= 0.0 || center >= 0.999999)
    {
        outColor = color;
        return;
    }

    int radius = int(clamp(round(edl.params.x), 1.0, 8.0));
    float depthScale = max(edl.params.z, 0.001);
    ivec2 offsets[8] = ivec2[](
        ivec2(+1, 0), ivec2(-1, 0), ivec2(0, +1), ivec2(0, -1),
        ivec2(+1, +1), ivec2(-1, +1), ivec2(+1, -1), ivec2(-1, -1));

    float response = 0.0;
    for (int i = 0; i < 8; i++)
    {
        ivec2 q = clamp(p + radius * offsets[i], ivec2(0), extent - ivec2(1));
        float neighbor = texelFetch(sampler2D(depthTex, samp), q, 0).r;
        response += max(0.0, neighbor - center) * depthScale;
    }
    response /= 8.0;

    float shade = exp(-max(edl.params.y, 0.0) * response);
    outColor = vec4(color.rgb * shade, color.a);
}
