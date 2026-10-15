#include "mesh_fragment_clip_vertex.glsl"
#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in uvec2 inIdentity;

layout(location = 0) flat out uint fragId;

layout(location = 1) out float fragAlpha;

void main()
{
    passFragmentClip();
    gl_Position = transform(inPos);
    fragId = inIdentity.x;
    fragAlpha = uintBitsToFloat(inIdentity.y);
}
