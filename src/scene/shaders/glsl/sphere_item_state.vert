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
layout(location = 3) out float fragSpriteRadiusPx;

void main()
{
    vec4 centerView = mvp.view * mvp.model * vec4(inPos, 1.0);
    vec4 tr = transform(inPos);
    float radius = max(transform_radius(applyItemStateScale(inSize, inItemState)), 1e-6);
    float radiusPx = sphereProjectedRadiusPx(centerView.xyz, radius);
    float paddedRadiusPx = radiusPx + 1.5;

    gl_Position = tr;
    gl_PointSize = max(2.0 * paddedRadiusPx, 1.0);
    fragColor = applyItemStateColor(inColor, inItemState);
    fragCenterView = centerView;
    fragRadius = radius;
    fragSpriteRadiusPx = paddedRadiusPx;
}
