#version 450

layout(set = 0, binding = 0) uniform texture2D occlusionTex;
layout(set = 0, binding = 1) uniform texture2D normalTex;
layout(set = 0, binding = 2) uniform texture2D depthTex;
layout(set = 0, binding = 3) uniform sampler samp;
layout(set = 0, binding = 4) uniform SsaoParams {
    mat4 invProj;
    mat4 view;
    vec4 viewport;
    vec4 params;
    vec4 params2;
    vec4 params3;
} ssao;
layout(location = 0) out float outVisibility;

vec3 reconstructViewPosition(ivec2 texel, float depth)
{
    vec2 uv = (vec2(texel) + vec2(0.5) - ssao.viewport.xy) / max(ssao.viewport.zw, vec2(1.0));
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc.x, -ndc.y, depth * 2.0 - 1.0, 1.0);
    vec4 view = ssao.invProj * clip;
    return view.xyz / max(abs(view.w), 1e-6);
}

vec3 viewNormalFromEncoded(vec4 normalSample)
{
    vec3 worldNormal = normalize(normalSample.xyz * 2.0 - 1.0);
    return normalize(mat3(ssao.view) * worldNormal);
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    ivec2 extent = textureSize(sampler2D(occlusionTex, samp), 0);
    if (p.x < 0 || p.y < 0 || p.x >= extent.x || p.y >= extent.y)
        discard;

    float centerDepth = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    if (centerDepth >= 0.999999)
    {
        outVisibility = 1.0;
        return;
    }

    vec3 centerPos = reconstructViewPosition(p, centerDepth);
    vec3 centerNormal = viewNormalFromEncoded(texelFetch(sampler2D(normalTex, samp), p, 0));
    int radius = int(clamp(round(ssao.params2.z), 1.0, 16.0));
    float depthSigma = max(ssao.params3.x, 0.001);
    float normalSigma = max(ssao.params3.y, 0.001);

    float weightedVisibility = 0.0;
    float totalWeight = 0.0;
    for (int y = -16; y <= 16; y++)
    {
        for (int x = -16; x <= 16; x++)
        {
            if (abs(x) > radius || abs(y) > radius)
                continue;
            ivec2 q = clamp(p + ivec2(x, y), ivec2(0), extent - ivec2(1));
            float sampleDepth = texelFetch(sampler2D(depthTex, samp), q, 0).r;
            if (sampleDepth >= 0.999999)
                continue;

            vec3 samplePos = reconstructViewPosition(q, sampleDepth);
            vec3 sampleNormal =
                viewNormalFromEncoded(texelFetch(sampler2D(normalTex, samp), q, 0));
            float spatial2 = float(x * x + y * y);
            float spatialWeight = exp(-spatial2 / max(2.0 * float(radius * radius), 1.0));
            float depthWeight = exp(-abs(samplePos.z - centerPos.z) / depthSigma);
            float normalWeight =
                smoothstep(1.0 - normalSigma, 1.0, dot(centerNormal, sampleNormal));
            float weight = spatialWeight * depthWeight * normalWeight;
            float visibility = texelFetch(sampler2D(occlusionTex, samp), q, 0).r;
            weightedVisibility += visibility * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 1e-6)
        outVisibility = texelFetch(sampler2D(occlusionTex, samp), p, 0).r;
    else
        outVisibility = clamp(weightedVisibility / totalWeight, 0.0, 1.0);
}
