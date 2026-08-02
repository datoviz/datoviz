#version 450

#include "scene_material.glsl"
#include "surface_depth.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 2) in float fragSize;
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

void main() {
    float dist = pointDiscDistance();
    float alpha = pointDiscCoverage(dist);
    if (alpha <= 0.0)
        discard;
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(applyDepthCue(linearColor.rgb, fragCue), linearColor.a * alpha);
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
