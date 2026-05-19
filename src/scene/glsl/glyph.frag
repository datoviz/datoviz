#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPixelRange()
{
    const float pixelRange = 4.0;
    vec2 unitRange = vec2(pixelRange) / vec2(textureSize(sampler2D(tex, samp), 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(fragUV);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main()
{
    vec4 texel = texture(sampler2D(tex, samp), fragUV);
    float opacity = texel.a;
    if (max(max(texel.r, texel.g), texel.b) > 1.0 / 255.0)
    {
        float sd = median3(texel.r, texel.g, texel.b);
        float trueSd = texel.a;
        if ((sd - 0.5) * (trueSd - 0.5) < 0.0)
            sd = trueSd;
        sd = clamp(sd, trueSd - 0.125, trueSd + 0.125);
        float screenPxDistance = screenPixelRange() * (sd - 0.5);
        opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    }
    outColor = vec4(fragColor.rgb, fragColor.a * opacity);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
