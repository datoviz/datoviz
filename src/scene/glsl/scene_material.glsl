#ifndef DVZ_SCENE_MATERIAL_GLSL
#define DVZ_SCENE_MATERIAL_GLSL

layout(set = 1, binding = 0) uniform SceneMaterial {
    vec4 lightDir;
    vec4 params;
    vec4 depthCue;
    vec4 depthCueColor;
    vec4 depthCueExtra;
} material;

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
