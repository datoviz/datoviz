#version 450

#include "color.glsl"
#include "surface_depth.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragSize;
layout(location = 0) out vec4 outColor;

#include "point_disc.glsl"

void main()
{
    float dist = pointDiscDistance();
    float alpha = pointDiscCoverage(dist);
    if (alpha <= 0.0)
        discard;
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(linearColor.rgb, linearColor.a * alpha);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
