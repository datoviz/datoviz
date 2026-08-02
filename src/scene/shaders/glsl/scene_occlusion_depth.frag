#version 450

#include "common.glsl"

#ifdef DVZ_SCENE_OCCLUSION_DEPTH_COLOR
layout(location = 0) in vec4 fragColor;
#endif

layout(location = 0) out float outDepth;

void main()
{
#ifdef DVZ_SCENE_OCCLUSION_DEPTH_COLOR
    if (fragColor.a <= 0.0) {
        discard;
    }
#endif
    outDepth = positiveLinearViewDepth(gl_FragCoord.z);
}
