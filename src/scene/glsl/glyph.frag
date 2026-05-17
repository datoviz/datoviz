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

void main()
{
    vec4 texel = texture(sampler2D(tex, samp), fragUV);
    float sd = median3(texel.r, texel.g, texel.b);
    float width = max(fwidth(sd), 1.0 / 255.0);
    float opacity = smoothstep(0.5 - width, 0.5 + width, sd);
    if (texel.a > 0.0 && abs(texel.a - sd) > 1.0 / 255.0)
        opacity = max(opacity, smoothstep(0.5 - width, 0.5 + width, texel.a));
    outColor = vec4(fragColor.rgb, fragColor.a * opacity);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
