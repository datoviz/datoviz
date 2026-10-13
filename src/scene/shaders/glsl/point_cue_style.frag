#version 450

#include "scene_material.glsl"
#include "surface_depth.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 2) in float fragSize;
layout(location = 0) out vec4 outColor;

#include "point_disc.glsl"

#include "point_style_surface.glsl"

void main()
{
    vec4 color = pointStyleSurface();
    outColor = vec4(applyDepthCue(color.rgb, fragCue), color.a);
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
