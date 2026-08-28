#version 450

#include "common.glsl"
#include "item_state_style.glsl"
#include "sphere_vertex.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;
layout(location = 5) in uint inItemState;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragCenterView;
layout(location = 2) out float fragRadius;
layout(location = 3) out vec2 fragNdc;

void main()
{
    vec4 centerView = mvp.view * mvp.model * vec4(inPos, 1.0);
    float radius = max(transform_radius(applyItemStateScale(inSize, inItemState)), 1e-6);
    vec4 clipPosition;
    vec2 ndc;
    sphereQuadVertex(centerView.xyz, radius, gl_VertexIndex, clipPosition, ndc);

    gl_Position = clipPosition;
    fragColor = applyItemStateColor(inColor, inItemState);
    fragCenterView = centerView;
    fragRadius = radius;
    fragNdc = ndc;
}
