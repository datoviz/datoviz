#ifndef DVZ_SURFACE_DEPTH_GLSL
#define DVZ_SURFACE_DEPTH_GLSL

#ifdef DVZ_SURFACE_DEPTH_OUTPUT
#include "common.glsl"

layout(location = 1) out float outSurfaceDepth;

void writeSurfaceDepthFromDevice()
{
    outSurfaceDepth = positiveLinearViewDepth(gl_FragCoord.z);
}

void writeSurfaceDepth(float linearDepth)
{
    outSurfaceDepth = max(linearDepth, 0.0);
}
#endif

#endif
