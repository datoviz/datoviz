#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragCenterView;
layout(location = 2) out float fragRadius;
layout(location = 3) out float fragSpriteScale;
layout(location = 4) flat out uint fragId;

void main()
{
    vec4 centerView = mvp.view * mvp.model * vec4(inPos, 1.0);
    vec4 tr = transform(inPos);
    float radius = max(transform_radius(inSize), 1e-6);
    float radiusPx =
        0.5 * radius * viewport.rect.w * abs(mvp.proj[0][0]) / max(abs(tr.w), 1e-6);
    float paddedRadiusPx = radiusPx + 1.5;

    gl_Position = tr;
    gl_PointSize = max(2.0 * paddedRadiusPx, 1.0);
    fragColor = inColor;
    fragCenterView = centerView;
    fragRadius = radius;
    fragSpriteScale = paddedRadiusPx / max(radiusPx, 1e-6);
    fragId = uint(gl_VertexIndex) + 1u;
}
