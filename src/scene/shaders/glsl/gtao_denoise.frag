#version 450

layout(set = 0, binding = 0) uniform texture2D visibilityTex;
layout(set = 0, binding = 1) uniform texture2D normalTex;
layout(set = 0, binding = 2) uniform texture2D depthTex;
layout(set = 0, binding = 3) uniform texture2D coverageTex;
layout(set = 0, binding = 4) uniform sampler samp;
layout(set = 0, binding = 5) uniform GtaoParams {
    mat4 proj;
    mat4 invProj;
    vec4 viewport;
    vec4 extent;
    vec4 appearance;
    vec4 sampling;
    ivec4 mode;
} gtao;

layout(location = 0) in vec2 localUv;
layout(location = 0) out float outVisibility;

#ifndef DVZ_GTAO_DENOISE_AXIS
#define DVZ_GTAO_DENOISE_AXIS 0
#endif

void main()
{
    ivec2 size = textureSize(sampler2D(visibilityTex, samp), 0);
    if (any(lessThanEqual(size, ivec2(0))))
    {
        outVisibility = 1.0;
        return;
    }
    ivec2 p = clamp(ivec2(localUv * vec2(size)), ivec2(0), size - ivec2(1));
    float centerCoverage = clamp(texelFetch(sampler2D(coverageTex, samp), p, 0).r, 0.0, 1.0);
    float centerDepth = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    vec3 centerNormal = texelFetch(sampler2D(normalTex, samp), p, 0).xyz;
    float centerVisibility = texelFetch(sampler2D(visibilityTex, samp), p, 0).r;
    if (centerCoverage <= 0.0 || !(centerDepth > 0.0) ||
        dot(centerNormal, centerNormal) <= 1e-8)
    {
        outVisibility = 1.0;
        return;
    }
    centerNormal = normalize(centerNormal);

    int axisIndex = DVZ_GTAO_DENOISE_AXIS != 0 ? DVZ_GTAO_DENOISE_AXIS : gtao.mode.z;
    ivec2 axis = axisIndex == 2 ? ivec2(0, 1) : ivec2(1, 0);
    float radius = max(gtao.appearance.x, 1e-4);
    float projected = 0.5 * radius *
                      max(abs(gtao.proj[0][0]) * float(size.x),
                          abs(gtao.proj[1][1]) * float(size.y));
    if (abs(gtao.proj[3][3]) < 0.5)
        projected /= max(centerDepth, 1e-4);
    int filterRadius = int(clamp(ceil(projected * 0.08), 1.0, 4.0));
    float depthSigma = max(gtao.sampling.z, 1e-4);
    float normalSigma = clamp(gtao.sampling.w, 1e-4, 1.0);

    float weightedVisibility = centerVisibility;
    float totalWeight = 1.0;
    for (int offset = -4; offset <= 4; offset++)
    {
        if (offset == 0 || abs(offset) > filterRadius)
            continue;
        ivec2 q = p + axis * offset;
        if (any(lessThan(q, ivec2(0))) || any(greaterThanEqual(q, size)))
            continue;
        float sampleCoverage =
            clamp(texelFetch(sampler2D(coverageTex, samp), q, 0).r, 0.0, 1.0);
        float sampleDepth = texelFetch(sampler2D(depthTex, samp), q, 0).r;
        vec3 sampleNormal = texelFetch(sampler2D(normalTex, samp), q, 0).xyz;
        if (sampleCoverage <= 0.0 || !(sampleDepth > 0.0) ||
            dot(sampleNormal, sampleNormal) <= 1e-8)
            continue;
        sampleNormal = normalize(sampleNormal);
        float spatial = exp(-0.5 * float(offset * offset) /
                            max(float(filterRadius * filterRadius), 1.0));
        float depthWeight = exp(-abs(sampleDepth - centerDepth) /
                                max(depthSigma * radius, 1e-4));
        float normalWeight =
            smoothstep(1.0 - normalSigma, 1.0, dot(centerNormal, sampleNormal));
        float weight = spatial * depthWeight * normalWeight * sampleCoverage;
        weightedVisibility += texelFetch(sampler2D(visibilityTex, samp), q, 0).r * weight;
        totalWeight += weight;
    }
    outVisibility = mix(1.0, clamp(weightedVisibility / totalWeight, 0.0, 1.0), centerCoverage);
}
