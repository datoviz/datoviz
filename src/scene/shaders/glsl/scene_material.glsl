#ifndef DVZ_SCENE_MATERIAL_GLSL
#define DVZ_SCENE_MATERIAL_GLSL

#include "color.glsl"

#ifdef DVZ_AMBIENT_VISIBILITY
#include "common.glsl"

layout(set = 3, binding = 0) uniform texture2D ambientVisibilityTex;
layout(set = 3, binding = 1) uniform sampler ambientVisibilitySamp;
#endif

layout(set = 1, binding = 0) uniform SceneMaterial {
    vec4 lightDir;
    vec4 params;
    vec4 model;
    vec4 baseColorFactor;
    vec4 standardParams;
    vec4 emissiveRim;
    vec4 limbParams;
    vec4 depthCue;
    vec4 depthCueColor;
    vec4 depthCueExtra;
} material;

float sceneAmbientVisibility()
{
#ifdef DVZ_AMBIENT_VISIBILITY
    vec2 viewportExtent = max(viewport.rect.zw, vec2(1.0));
    vec2 localUv = (gl_FragCoord.xy - viewport.rect.xy) / viewportExtent;
    ivec2 productExtent = textureSize(sampler2D(ambientVisibilityTex, ambientVisibilitySamp), 0);
    vec2 halfTexel = 0.5 / max(vec2(productExtent), vec2(1.0));
    localUv = clamp(localUv, halfTexel, vec2(1.0) - halfTexel);
    return clamp(
        texture(sampler2D(ambientVisibilityTex, ambientVisibilitySamp), localUv).r, 0.0, 1.0);
#else
    return 1.0;
#endif
}

vec4 evaluateSceneMaterialLinearItemWithAmbientVisibility(
    vec4 linearItemColor, vec3 normal, vec3 worldPos, vec3 cameraPos, float ambientVisibility)
{
    int model = int(material.model.x + 0.5);
    float opacity = clamp(material.model.y, 0.0, 1.0);
    vec4 linearBaseColor = semanticColorToLinear(material.baseColorFactor);
    vec3 emissive = srgbToLinear(clamp(material.emissiveRim.rgb, 0.0, 1.0));
    vec3 base = linearItemColor.rgb * linearBaseColor.rgb;
    float alpha = linearItemColor.a * linearBaseColor.a * opacity;
    if (model == 0)
        return vec4(clamp(base + emissive, 0.0, 1.0), alpha);

    vec3 n = normalize(normal);
    vec3 l = normalize(material.lightDir.xyz);
    vec3 v = normalize(cameraPos - worldPos);
    if (model == 3)
    {
        float falloff = max(material.limbParams.x, 0.01);
        float sunBias = material.limbParams.y;
        float terminatorWidth = max(material.limbParams.z, 1e-4);
        float nightFactor = clamp(material.limbParams.w, 0.0, 1.0);
        float facing = clamp(dot(n, v), 0.0, 1.0);
        float peakFacing = 2.0 / (falloff + 2.0);
        float peak = peakFacing * peakFacing * pow(1.0 - peakFacing, falloff);
        float limb = facing * facing * pow(1.0 - facing, falloff) / max(peak, 1e-6);
        float sunlight = smoothstep(
            -terminatorWidth, terminatorWidth, dot(n, l) + sunBias);
        float illumination = mix(nightFactor, 1.0, sunlight);
        return vec4(clamp(base, 0.0, 1.0), alpha * limb * illumination);
    }
    vec3 h = normalize(l + v);
    float lambert = max(dot(n, l), 0.0);
    if (model == 2)
    {
        float roughness = clamp(material.standardParams.x, 0.0, 1.0);
        float specularStrength = max(material.standardParams.y, 0.0);
        float metallic = clamp(material.standardParams.z, 0.0, 1.0);
        float rimStrength = max(material.standardParams.w, 0.0);
        float shininess = max(1.0, 128.0 * (1.0 - roughness) + 1.0);
        float spec = pow(max(dot(n, h), 0.0), shininess) * specularStrength;
        float rim = pow(1.0 - max(dot(n, v), 0.0), 2.0) * rimStrength;
        float visibility = clamp(ambientVisibility, 0.0, 1.0);
        vec3 diffuse = base * (0.04 * visibility + (1.0 - metallic) * lambert);
        vec3 rgb = diffuse + vec3(spec + rim) + emissive;
        return vec4(clamp(rgb, 0.0, 1.0), alpha);
    }

    float spec = pow(max(dot(n, h), 0.0), max(material.params.w, 1.0));
    float visibility = clamp(ambientVisibility, 0.0, 1.0);
    vec3 rgb = base * (material.params.x * visibility + material.params.y * lambert) +
               vec3(material.params.z * spec);
    return vec4(clamp(rgb, 0.0, 1.0), alpha);
}

vec4 evaluateSceneMaterialLinearItem(vec4 linearItemColor, vec3 normal, vec3 worldPos, vec3 cameraPos)
{
    return evaluateSceneMaterialLinearItemWithAmbientVisibility(
        linearItemColor, normal, worldPos, cameraPos, sceneAmbientVisibility());
}

vec4 evaluateSceneMaterialWithAmbientVisibility(
    vec4 itemColor, vec3 normal, vec3 worldPos, vec3 cameraPos, float ambientVisibility)
{
    return evaluateSceneMaterialLinearItemWithAmbientVisibility(
        semanticColorToLinear(itemColor), normal, worldPos, cameraPos, ambientVisibility);
}

vec4 evaluateSceneMaterial(vec4 itemColor, vec3 normal, vec3 worldPos, vec3 cameraPos)
{
    return evaluateSceneMaterialWithAmbientVisibility(
        itemColor, normal, worldPos, cameraPos, sceneAmbientVisibility());
}

float depthCueCoordinate(vec3 cue)
{
    int metric = int(material.depthCueExtra.x + 0.5);
    if (metric == 1)
        return cue.y;
    if (metric == 2)
        return cue.z;
    return cue.x;
}

float depthCueFactor(vec3 cue)
{
    float strength = clamp(material.depthCue.z, 0.0, 1.0);
    int mode = int(material.depthCue.w + 0.5);
    if (mode == 0 || strength <= 0.0)
        return 0.0;

    float denom = max(material.depthCue.y - material.depthCue.x, 1e-6);
    float coord = depthCueCoordinate(cue);
    float t = clamp((coord - material.depthCue.x) / denom, 0.0, 1.0);
    int falloff = int(material.depthCueExtra.y + 0.5);
    if (falloff == 1)
    {
        float density = max(material.depthCueExtra.z, 1e-6);
        t = (1.0 - exp(-density * t)) / max(1.0 - exp(-density), 1e-6);
    }
    return t * strength;
}

vec3 applyDepthCue(vec3 rgb, vec3 cue)
{
    int mode = int(material.depthCue.w + 0.5);
    float t = depthCueFactor(cue);
    if (t <= 0.0)
        return rgb;
    if (mode == 1)
        return mix(rgb, srgbToLinear(clamp(material.depthCueColor.rgb, 0.0, 1.0)), t);
    if (mode == 2)
    {
        float luma = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
        return mix(rgb, vec3(luma), t);
    }
    if (mode == 3)
        return mix(rgb, vec3(0.0), t);
    return rgb;
}

#endif
