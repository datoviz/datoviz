#version 450

#include "common.glsl"
#include "scene_material.glsl"
#include "sphere_analytic.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragCenterView;
layout(location = 2) in float fragRadius;
layout(location = 3) in vec2 fragNdc;
layout(location = 0) out vec4 outDepth;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out float outCoverage;

float coverageThreshold(vec2 fragCoord)
{
    return fract(52.9829189 * fract(0.06711056 * fragCoord.x + 0.00583715 * fragCoord.y));
}

void main()
{
    DvzSphereHit hit;
    if (!sphereIntersect(fragCenterView, fragRadius, fragNdc, hit))
        discard;
    float coverage = hit.coverage * fragColor.a;
#ifdef DVZ_SURFACE_CAPTURE_A2C
    if (coverage <= 0.0)
        discard;
#else
    if (coverage <= coverageThreshold(gl_FragCoord.xy))
        discard;
    coverage = 1.0;
#endif
    gl_FragDepth = hit.deviceDepth;
    outDepth = vec4(hit.linearDepth, 0.0, 0.0, coverage);
    outNormal = vec4(hit.normalView, 0.0);
    outCoverage = coverage;
}
