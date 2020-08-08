#version 450
#include "../../src/shaders/common.glsl"

layout (std140, binding = 2) uniform Params {
    vec2 mouse;
    float time;
    int frame;
} params;

layout (binding = 3) uniform sampler2D iChannel0;

#define iResolution (mvp.viewport.zw)
#define iMouse (params.mouse)
#define iTime (params.time)
#define iFrame (params.frame)

layout (location = 0) in vec2 in_coords;
layout (location = 0) out vec4 out_color;

/* SHADERTOY BEGIN */
#include "shadertoy.glsl"
/* SHADERTOY END */

void main()
{
    mainImage(out_color, in_coords * (mvp.viewport.z, -mvp.viewport.w));
}
