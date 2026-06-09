#version 450

#include "common.glsl"
#include "item_state_style.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 3) in vec4 inInstanceTransform0;
layout(location = 4) in vec4 inInstanceTransform1;
layout(location = 5) in vec4 inInstanceTransform2;
layout(location = 6) in vec4 inInstanceTransform3;
layout(location = 7) in uint inItemState;

layout(location = 0) out vec4 fragColor;

void main()
{
    mat4 instanceTransform = mat4(
        inInstanceTransform0,
        inInstanceTransform1,
        inInstanceTransform2,
        inInstanceTransform3);
    mat4 model = mvp.model * instanceTransform;
    vec4 local = vec4(inPos, 1.0);
    local.xyz *= applyItemStateScale(1.0, inItemState);
    vec4 world = model * local;
    vec4 tr = mvp.proj * mvp.view * world;
    tr.y = -tr.y;
    tr.z = 0.5 * (tr.z + tr.w);
    gl_Position = tr;
    fragColor = applyItemStateColor(inColor, inItemState);
}
