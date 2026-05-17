#version 450

#include "scene_material.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragSize;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float dist = length(uv);
    float aa = max(fwidth(dist), 1e-6);
    float outer = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, dist);
    if (outer <= 0.0)
        discard;

    float lineWidth = max(material.params.x, 0.0);
    bool filled = material.params.y > 0.5;
    bool stroke = material.params.z > 0.5 || material.params.w > 0.5;
    bool outline = material.params.w > 0.5;
    float strokeWidth = stroke ? max(lineWidth, 1.0) : 0.0;
    float innerRadius = max(1.0 - 2.0 * strokeWidth / max(fragSize, 1.0), 0.0);
    float edgeMix = stroke ? smoothstep(innerRadius - aa, innerRadius + aa, dist) : 0.0;
    float fillMask = (filled && !outline) ? 1.0 - edgeMix : 0.0;
    float strokeMask = stroke ? edgeMix : 0.0;
    float coverage = outer * max(fillMask, strokeMask);
    if (coverage <= 0.0)
        discard;

    vec4 edgeColor = material.baseColorFactor;
    vec4 color = mix(fragColor, edgeColor, strokeMask);
    outColor = vec4(color.rgb, color.a * coverage);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
