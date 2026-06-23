#version 450

#include "scene_material.glsl"

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
    float aa = max(fwidth(dist), 1e-6);
    return 1.0 - smoothstep(-aa, aa, dist);
}

void main()
{
    float dist = pointDiscDistance();
    float aa = max(fwidth(dist), 1e-6);
    float outer = pointDiscCoverage(dist);
    if (outer <= 0.0)
        discard;

    float lineWidth = max(material.params.x, 0.0);
    int aspect = int(material.params.y + 0.5);
    bool filled = aspect == 0 || aspect == 2;
    bool stroke = (aspect == 1 || aspect == 2) && lineWidth > 0.0;
    float strokeWidth = stroke ? lineWidth : 0.0;
    float edgeMix = stroke ? smoothstep(-aa, aa, dist + strokeWidth) : 0.0;
    float fillMask = filled ? 1.0 - edgeMix : 0.0;
    float strokeMask = stroke ? edgeMix : 0.0;
    float coverage = outer * max(fillMask, strokeMask);
    if (coverage <= 0.0)
        discard;

    vec4 edgeColor = semanticColorToLinear(material.baseColorFactor);
    vec4 linearColor = semanticColorToLinear(fragColor);
    vec4 color = filled ? mix(linearColor, edgeColor, strokeMask) : edgeColor;
    outColor = vec4(color.rgb, color.a * coverage);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
