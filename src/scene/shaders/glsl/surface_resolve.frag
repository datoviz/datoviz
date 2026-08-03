#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(set = 0, binding = 0) uniform texture2DMS depthTex;
layout(set = 0, binding = 1) uniform texture2DMS normalTex;
layout(set = 0, binding = 2) uniform texture2DMS coverageTex;
layout(location = 0) in vec2 localUv;
layout(location = 0) out float outDepth;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out float outCoverage;

void main()
{
    ivec2 extent = textureSize(depthTex);
    ivec2 texel = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));
    int sampleCount = textureSamples(depthTex);

    float winningDepth = 0.0;
    vec4 winningNormal = vec4(0.0, 0.0, 1.0, 0.0);
    float coveredSamples = 0.0;
    for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
    {
        float coverage = clamp(texelFetch(coverageTex, texel, sampleIndex).r, 0.0, 1.0);
        float depth = texelFetch(depthTex, texel, sampleIndex).r;
        if (coverage > 0.0)
            coveredSamples += 1.0;
        if (coverage <= 0.0 || depth <= 0.0 || isnan(depth) || isinf(depth))
            continue;
        if (winningDepth == 0.0 || depth < winningDepth)
        {
            winningDepth = depth;
            winningNormal = texelFetch(normalTex, texel, sampleIndex);
        }
    }

    outDepth = winningDepth;
    outNormal = winningDepth > 0.0 ? winningNormal : vec4(0.0, 0.0, 1.0, 0.0);
    outCoverage = coveredSamples / max(float(sampleCount), 1.0);
}
