#version 450

layout(set = 0, binding = 0) uniform texture2D normalTex;
layout(set = 0, binding = 1) uniform texture2D depthTex;
layout(set = 0, binding = 2) uniform texture2D coverageTex;
layout(set = 0, binding = 3) uniform sampler samp;
layout(set = 0, binding = 4) uniform GtaoParams {
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

const float PI = 3.14159265358979323846;
const int MAX_DIRECTIONS = 6;
const int MAX_STEPS = 8;

bool reconstructViewPosition(ivec2 texel, ivec2 size, float depth, out vec3 position)
{
    if (!(depth > 0.0) || isnan(depth) || isinf(depth))
        return false;
    vec2 uv = (vec2(texel) + vec2(0.5)) / max(vec2(size), vec2(1.0));
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 a4 = gtao.invProj * vec4(ndc.x, -ndc.y, -1.0, 1.0);
    vec4 b4 = gtao.invProj * vec4(ndc.x, -ndc.y, 1.0, 1.0);
    if (abs(a4.w) <= 1e-8 || abs(b4.w) <= 1e-8)
        return false;
    vec3 a = a4.xyz / a4.w;
    vec3 b = b4.xyz / b4.w;
    float dz = b.z - a.z;
    if (abs(dz) <= 1e-8)
        return false;
    position = a + (b - a) * ((-depth - a.z) / dz);
    return !any(isnan(position)) && !any(isinf(position));
}

float projectedRadiusPixels(float radius, float depth, ivec2 size)
{
    vec2 projected = 0.5 * radius * abs(vec2(gtao.proj[0][0], gtao.proj[1][1])) * vec2(size);
    if (abs(gtao.proj[3][3]) < 0.5)
        projected /= max(depth, 1e-4);
    return clamp(max(projected.x, projected.y), 1.0, float(max(size.x, size.y)));
}

void main()
{
    ivec2 size = textureSize(sampler2D(depthTex, samp), 0);
    if (any(lessThanEqual(size, ivec2(0))))
    {
        outVisibility = 1.0;
        return;
    }
    ivec2 p = clamp(ivec2(localUv * vec2(size)), ivec2(0), size - ivec2(1));
    float centerCoverage = clamp(texelFetch(sampler2D(coverageTex, samp), p, 0).r, 0.0, 1.0);
    float centerDepth = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    vec3 centerPosition = vec3(0.0);
    vec3 centerNormal = texelFetch(sampler2D(normalTex, samp), p, 0).xyz;
    if (centerCoverage <= 0.0 || dot(centerNormal, centerNormal) <= 1e-8 ||
        !reconstructViewPosition(p, size, centerDepth, centerPosition))
    {
        outVisibility = 1.0;
        return;
    }
    centerNormal = normalize(centerNormal);

    float radius = max(gtao.appearance.x, 1e-4);
    float intensity = max(gtao.appearance.y, 0.0);
    float thickness = max(gtao.appearance.z, 1e-4);
    float minVisibility = clamp(gtao.appearance.w, 0.0, 1.0);
    float falloffScale = max(gtao.sampling.x, radius);
    float bias = max(gtao.sampling.y, 0.0);
    int directionCount = clamp(gtao.mode.x, 1, MAX_DIRECTIONS);
    int stepCount = clamp(gtao.mode.y, 1, MAX_STEPS);
    float radiusPixels = projectedRadiusPixels(radius, centerDepth, size);

    float horizonSum = 0.0;
    for (int directionIndex = 0; directionIndex < MAX_DIRECTIONS; directionIndex++)
    {
        if (directionIndex >= directionCount)
            break;
        float theta = PI * (float(directionIndex) + 0.5) / float(directionCount);
        vec2 axis = vec2(cos(theta), sin(theta));
        for (int side = -1; side <= 1; side += 2)
        {
            float rayHorizon = 0.0;
            for (int stepIndex = 0; stepIndex < MAX_STEPS; stepIndex++)
            {
                if (stepIndex >= stepCount)
                    break;
                float t = (float(stepIndex) + 1.0) / float(stepCount);
                ivec2 q = p + ivec2(round(axis * float(side) * radiusPixels * t));
                if (any(lessThan(q, ivec2(0))) || any(greaterThanEqual(q, size)))
                    continue;
                float sampleCoverage =
                    clamp(texelFetch(sampler2D(coverageTex, samp), q, 0).r, 0.0, 1.0);
                float sampleDepth = texelFetch(sampler2D(depthTex, samp), q, 0).r;
                vec3 samplePosition = vec3(0.0);
                if (sampleCoverage <= 0.0 ||
                    !reconstructViewPosition(q, size, sampleDepth, samplePosition))
                    continue;
                vec3 delta = samplePosition - centerPosition;
                float distanceToSample = length(delta);
                if (distanceToSample <= 1e-5 || distanceToSample > radius + thickness)
                    continue;
                float angular = max(dot(centerNormal, delta / distanceToSample) - bias, 0.0);
                float distanceWeight =
                    1.0 - smoothstep(radius, radius + thickness, distanceToSample);
                float falloff = 1.0 - smoothstep(0.0, falloffScale, distanceToSample);
                rayHorizon = max(rayHorizon, angular * distanceWeight * falloff * sampleCoverage);
            }
            horizonSum += rayHorizon;
        }
    }

    float rawVisibility = clamp(1.0 - horizonSum / (2.0 * float(directionCount)), 0.0, 1.0);
    float mappedVisibility = clamp(1.0 - intensity * (1.0 - rawVisibility), minVisibility, 1.0);
    outVisibility = mix(1.0, mappedVisibility, centerCoverage);
}
