#ifndef DVZ_SCENE_MATERIAL_GLSL
#define DVZ_SCENE_MATERIAL_GLSL

layout(set = 1, binding = 0) uniform SceneMaterial {
    vec4 lightDir;
    vec4 params;
    vec4 depthCue;
    vec4 depthCueColor;
} material;

float depthCueFactor(float depth)
{
    float strength = clamp(material.depthCue.z, 0.0, 1.0);
    int mode = int(material.depthCue.w + 0.5);
    if (mode == 0 || strength <= 0.0)
        return 0.0;

    float denom = max(material.depthCue.y - material.depthCue.x, 1e-6);
    return clamp((depth - material.depthCue.x) / denom, 0.0, 1.0) * strength;
}

vec3 applyDepthCue(vec3 rgb, float depth)
{
    int mode = int(material.depthCue.w + 0.5);
    float t = depthCueFactor(depth);
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
