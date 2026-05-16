#ifndef DVZ_SCENE_MATERIAL_GLSL
#define DVZ_SCENE_MATERIAL_GLSL

layout(set = 1, binding = 0) uniform SceneMaterial {
    vec4 lightDir;
    vec4 params;
    vec4 model;
    vec4 baseColorFactor;
    vec4 standardParams;
    vec4 emissiveRim;
    vec4 depthCue;
    vec4 depthCueColor;
    vec4 depthCueExtra;
} material;

vec4 evaluateSceneMaterial(vec4 itemColor, vec3 normal, vec3 worldPos, vec3 cameraPos)
{
    int model = int(material.model.x + 0.5);
    float opacity = clamp(material.model.y, 0.0, 1.0);
    vec3 base = itemColor.rgb * material.baseColorFactor.rgb;
    float alpha = itemColor.a * material.baseColorFactor.a * opacity;
    if (model == 0)
        return vec4(clamp(base + material.emissiveRim.rgb, 0.0, 1.0), alpha);

    vec3 n = normalize(normal);
    vec3 l = normalize(material.lightDir.xyz);
    vec3 v = normalize(cameraPos - worldPos);
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
        vec3 diffuse = base * (0.04 + (1.0 - metallic) * lambert);
        vec3 rgb = diffuse + vec3(spec + rim) + material.emissiveRim.rgb;
        return vec4(clamp(rgb, 0.0, 1.0), alpha);
    }

    float spec = pow(max(dot(n, h), 0.0), max(material.params.w, 1.0));
    vec3 rgb = base * (material.params.x + material.params.y * lambert) +
               vec3(material.params.z * spec);
    return vec4(clamp(rgb, 0.0, 1.0), alpha);
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
        return mix(rgb, material.depthCueColor.rgb, t);
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
