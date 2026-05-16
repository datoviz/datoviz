#version 450

layout(set = 1, binding = 0) uniform PrimitiveShading {
    vec4 lightDir;
    vec4 params;
    vec4 depthCue;
    vec4 depthCueColor;
} shading;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragDepth;
layout(location = 0) out vec4 outColor;

vec3 applyDepthCue(vec3 rgb)
{
    float strength = clamp(shading.depthCue.z, 0.0, 1.0);
    int mode = int(shading.depthCue.w + 0.5);
    if (mode == 0 || strength <= 0.0)
        return rgb;

    float denom = max(shading.depthCue.y - shading.depthCue.x, 1e-6);
    float t = clamp((fragDepth - shading.depthCue.x) / denom, 0.0, 1.0) * strength;
    if (mode == 1)
        return mix(rgb, shading.depthCueColor.rgb, t);
    if (mode == 2)
    {
        float luma = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
        return mix(rgb, vec3(luma), t);
    }
    if (mode == 3)
        return mix(rgb, vec3(0.0), t);
    return rgb;
}

void main() {
    outColor = vec4(applyDepthCue(fragColor.rgb), fragColor.a);
}
