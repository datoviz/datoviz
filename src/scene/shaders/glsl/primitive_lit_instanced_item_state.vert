#version 450

#include "common.glsl"
#include "item_state_style.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inInstanceTransform0;
layout(location = 4) in vec4 inInstanceTransform1;
layout(location = 5) in vec4 inInstanceTransform2;
layout(location = 6) in vec4 inInstanceTransform3;
layout(location = 7) in uint inItemState;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragCameraPos;
layout(location = 4) out float fragDepth;

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
    vec4 tr = sceneClipToDeviceClip(mvp.proj * mvp.view * world);
    gl_Position = tr;
    fragColor = applyItemStateColor(inColor, inItemState);
    fragWorldPos = world.xyz;
    fragCameraPos = (inverse(mvp.view) * vec4(0, 0, 0, 1)).xyz;
    fragNormal = transpose(inverse(mat3(model))) * inNormal;
    fragDepth = tr.z / max(abs(tr.w), 1e-6);
}
