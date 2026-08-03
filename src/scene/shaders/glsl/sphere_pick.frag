#version 450

#include "common.glsl"
#include "sphere_analytic.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragCenterView;
layout(location = 2) in float fragRadius;
layout(location = 3) in float fragSpriteRadiusPx;
layout(location = 0) out vec4 outColor;

void main()
{
    DvzSphereHit hit;
    if (!sphereIntersect(fragCenterView, fragRadius, fragSpriteRadiusPx, hit))
        discard;
    gl_FragDepth = hit.deviceDepth;
    outColor = fragColor;
}
