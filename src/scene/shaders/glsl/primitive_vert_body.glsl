#include "mesh_fragment_clip_vertex.glsl"
#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main()
{
    passFragmentClip();
    gl_Position = transform(inPos);
    fragColor = inColor;
}
