#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(set = 0, binding = 1) uniform Viewport {
    vec4 rect;
} viewport;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragCenterView;
layout(location = 2) out float fragRadius;

vec4 transform(vec3 pos)
{
    vec4 tr = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    tr.y = -tr.y;
    tr.z = 0.5 * (tr.z + tr.w);
    return tr;
}

void main()
{
    vec4 centerView = mvp.view * mvp.model * vec4(inPos, 1.0);
    vec4 tr = transform(inPos);
    float radius = max(inSize, 1e-6);
    float diameterPx = 2.0 * radius * viewport.rect.w * abs(mvp.proj[1][1]) /
                       max(abs(tr.w), 1e-6);

    gl_Position = tr;
    gl_PointSize = max(diameterPx, 1.0);
    fragColor = inColor;
    fragCenterView = centerView;
    fragRadius = radius;
}
