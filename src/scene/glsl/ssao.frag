#version 450

layout(set = 0, binding = 0) uniform texture2D normalTex;
layout(set = 0, binding = 1) uniform texture2D depthTex;
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform SsaoParams {
    vec4 params;
} ssao;
layout(location = 0) out float outOcclusion;

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    ivec2 extent = textureSize(sampler2D(depthTex, samp), 0);
    if (p.x < 0 || p.y < 0 || p.x >= extent.x || p.y >= extent.y)
        discard;

    float centerDepth = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    if (centerDepth >= 0.999999)
    {
        outOcclusion = 1.0;
        return;
    }

    vec3 centerNormal = texelFetch(sampler2D(normalTex, samp), p, 0).xyz * 2.0 - 1.0;
    centerNormal = normalize(centerNormal);

    vec2 kernel[16] = vec2[](
        vec2(+1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, +1.0), vec2(0.0, -1.0),
        vec2(+0.7, +0.7), vec2(-0.7, +0.7), vec2(+0.7, -0.7), vec2(-0.7, -0.7),
        vec2(+1.5, +0.5), vec2(-1.5, +0.5), vec2(+0.5, +1.5), vec2(+0.5, -1.5),
        vec2(+2.0, 0.0), vec2(-2.0, 0.0), vec2(0.0, +2.0), vec2(0.0, -2.0));

    float radius = max(ssao.params.x, 0.001);
    float strength = max(ssao.params.y, 0.0);
    float bias = max(ssao.params.z, 0.0);
    int sampleCount = int(clamp(round(ssao.params.w), 4.0, 16.0));
    float pixelRadius = clamp(radius * 8.0, 1.0, 64.0);
    float depthRadius = max(radius * 0.05, 1e-5);

    float occlusion = 0.0;
    for (int i = 0; i < sampleCount; i++)
    {
        ivec2 q = clamp(p + ivec2(round(kernel[i] * pixelRadius)), ivec2(0), extent - ivec2(1));
        float sampleDepth = texelFetch(sampler2D(depthTex, samp), q, 0).r;
        if (sampleDepth >= 0.999999)
            continue;
        vec3 sampleNormal = texelFetch(sampler2D(normalTex, samp), q, 0).xyz * 2.0 - 1.0;
        sampleNormal = normalize(sampleNormal);
        float closer = centerDepth - sampleDepth - bias;
        float depthWeight = smoothstep(0.0, depthRadius, closer);
        float normalWeight = clamp(1.0 - dot(centerNormal, sampleNormal), 0.0, 1.0);
        occlusion += depthWeight * (0.35 + 0.65 * normalWeight);
    }

    float visibility = 1.0 - occlusion / float(sampleCount);
    outOcclusion = clamp(pow(max(visibility, 0.0), strength), 0.0, 1.0);
}
