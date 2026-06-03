#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragSize;
layout(location = 0) out vec4 outColor;

float pointDiscDistance()
{
    float size = max(fragSize, 0.0);
    float spriteSize = max(size + 4.0, 1.0);
    vec2 p = gl_PointCoord.xy - vec2(0.5);
    return length(p * spriteSize) - 0.5 * size;
}

float pointDiscCoverage(float dist)
{
    if (dist <= 0.0)
        return 1.0;
    return exp(-dist * dist);
}

void main()
{
    float dist = pointDiscDistance();
    float alpha = pointDiscCoverage(dist);
    if (alpha < 0.05)
        discard;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
