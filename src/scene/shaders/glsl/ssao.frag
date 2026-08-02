#version 450

layout(set = 0, binding = 0) uniform texture2D normalTex;
layout(set = 0, binding = 1) uniform texture2D depthTex;
layout(set = 0, binding = 2) uniform texture2D coverageTex;
layout(set = 0, binding = 3) uniform sampler samp;
layout(set = 0, binding = 4) uniform SsaoParams {
    mat4 invProj;
    mat4 view;
    vec4 viewport;
    vec4 params;
    vec4 params2;
    vec4 params3;
} ssao;
layout(location = 0) in vec2 localUv;
layout(location = 0) out float outVisibility;

const vec3 KERNEL[32] = vec3[](
    vec3(+0.078, +0.016, +0.028), vec3(-0.056, +0.065, +0.052),
    vec3(+0.016, -0.093, +0.083), vec3(+0.105, +0.091, +0.118),
    vec3(-0.133, -0.045, +0.152), vec3(+0.161, -0.128, +0.196),
    vec3(-0.190, +0.151, +0.248), vec3(+0.226, +0.033, +0.309),
    vec3(-0.027, -0.281, +0.344), vec3(+0.295, -0.201, +0.392),
    vec3(-0.335, +0.221, +0.446), vec3(+0.094, +0.414, +0.492),
    vec3(-0.429, -0.125, +0.544), vec3(+0.461, +0.275, +0.598),
    vec3(-0.206, +0.535, +0.641), vec3(+0.549, -0.354, +0.682),
    vec3(-0.609, -0.321, +0.717), vec3(+0.342, +0.622, +0.741),
    vec3(-0.107, -0.704, +0.772), vec3(+0.693, +0.088, +0.795),
    vec3(-0.653, +0.413, +0.819), vec3(+0.237, -0.765, +0.842),
    vec3(-0.421, -0.681, +0.861), vec3(+0.792, -0.261, +0.879),
    vec3(-0.806, +0.097, +0.894), vec3(+0.546, +0.613, +0.910),
    vec3(-0.258, +0.790, +0.927), vec3(+0.049, -0.862, +0.941),
    vec3(-0.699, -0.493, +0.954), vec3(+0.888, +0.351, +0.966),
    vec3(-0.942, -0.023, +0.978), vec3(+0.338, -0.893, +0.990));

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 reconstructViewPosition(ivec2 texel, ivec2 extent, float depth)
{
    vec2 uv = (vec2(texel) + vec2(0.5)) / max(vec2(extent), vec2(1.0));
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc.x, -ndc.y, depth * 2.0 - 1.0, 1.0);
    vec4 view = ssao.invProj * clip;
    return view.xyz / max(abs(view.w), 1e-6);
}

bool projectViewPosition(vec3 viewPos, ivec2 extent, out ivec2 texel)
{
    mat4 proj = inverse(ssao.invProj);
    vec4 clip = proj * vec4(viewPos, 1.0);
    if (abs(clip.w) <= 1e-6)
        return false;
    clip.y = -clip.y;
    vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0)
        return false;
    texel = clamp(ivec2(uv * vec2(extent)), ivec2(0), extent - ivec2(1));
    return true;
}

vec3 viewNormalFromEncoded(vec4 normalSample)
{
    vec3 worldNormal = normalize(normalSample.xyz * 2.0 - 1.0);
    return normalize(mat3(ssao.view) * worldNormal);
}

void tangentBasis(vec3 normal, vec2 fragCoord, out vec3 tangent, out vec3 bitangent)
{
    float angle = 6.28318530718 * hash12(fragCoord);
    vec3 randomVec = normalize(vec3(cos(angle), sin(angle), 0.37));
    tangent = randomVec - normal * dot(randomVec, normal);
    if (dot(tangent, tangent) < 1e-5)
        tangent = abs(normal.z) < 0.9 ? cross(normal, vec3(0.0, 0.0, 1.0)) :
                                       cross(normal, vec3(0.0, 1.0, 0.0));
    tangent = normalize(tangent);
    bitangent = normalize(cross(normal, tangent));
}

void main()
{
    ivec2 extent = textureSize(sampler2D(depthTex, samp), 0);
    ivec2 p = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));

    float centerCoverage = texelFetch(sampler2D(coverageTex, samp), p, 0).r;
    if (centerCoverage <= 0.0)
    {
        outVisibility = 1.0;
        return;
    }

    float centerDepth = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    if (centerDepth >= 0.999999)
    {
        outVisibility = 1.0;
        return;
    }

    vec4 centerSample = texelFetch(sampler2D(normalTex, samp), p, 0);
    vec3 centerPos = reconstructViewPosition(p, extent, centerDepth);
    vec3 centerNormal = viewNormalFromEncoded(centerSample);

    vec3 tangent = vec3(0.0);
    vec3 bitangent = vec3(0.0);
    tangentBasis(centerNormal, vec2(p) + vec2(0.5), tangent, bitangent);

    float radius = max(ssao.params.x, 0.001);
    float bias = max(ssao.params.z, 0.0);
    int sampleCount = int(clamp(round(ssao.params.w), 4.0, 32.0));

    float occlusion = 0.0;
    float validSamples = 0.0;
    for (int i = 0; i < sampleCount; i++)
    {
        vec3 k = KERNEL[i];
        vec3 sampleVec = normalize(tangent * k.x + bitangent * k.y + centerNormal * k.z);
        float sampleScale = 0.18 + 0.82 * (float(i) + 1.0) / float(sampleCount);
        sampleScale *= sampleScale;
        vec3 samplePos = centerPos + sampleVec * radius * sampleScale;

        ivec2 q = ivec2(0);
        if (!projectViewPosition(samplePos, extent, q))
            continue;
        float sampleDepth = texelFetch(sampler2D(depthTex, samp), q, 0).r;
        if (sampleDepth >= 0.999999)
            continue;

        vec4 sampleNormalDepth = texelFetch(sampler2D(normalTex, samp), q, 0);
        vec3 actualPos = reconstructViewPosition(q, extent, sampleDepth);
        vec3 actualNormal = viewNormalFromEncoded(sampleNormalDepth);
        float distanceWeight = 1.0 - smoothstep(0.0, radius, length(actualPos - centerPos));
        float facingWeight = smoothstep(0.0, 0.6, dot(centerNormal, actualNormal) * 0.5 + 0.5);
        float blocker = actualPos.z >= samplePos.z + bias ? 1.0 : 0.0;
        occlusion += blocker * distanceWeight * facingWeight;
        validSamples += 1.0;
    }

    float rawVisibility = 1.0;
    if (validSamples > 0.5)
        rawVisibility = 1.0 - occlusion / validSamples;
    outVisibility = clamp(rawVisibility, 0.0, 1.0);
}
