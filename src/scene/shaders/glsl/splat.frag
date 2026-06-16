#version 450

#include "color.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragOffsetPx;
layout(location = 2) in vec2 fragSigma;
layout(location = 3) in float fragAngle;

layout(location = 0) out vec4 outColor;

const float CUTOFF_SIGMA = 3.0;

void main()
{
    float c = cos(fragAngle);
    float s = sin(fragAngle);
    vec2 local = vec2(
        c * fragOffsetPx.x + s * fragOffsetPx.y,
        -s * fragOffsetPx.x + c * fragOffsetPx.y);
    vec2 q = local / max(fragSigma, vec2(0.000001));
    float q2 = dot(q, q);
    if (q2 > CUTOFF_SIGMA * CUTOFF_SIGMA)
        discard;

    float alpha = fragColor.a * exp(-0.5 * q2);
    if (alpha <= 0.0)
        discard;

    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(linearColor.rgb, alpha);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
