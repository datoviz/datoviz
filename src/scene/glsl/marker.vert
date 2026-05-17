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
layout(location = 3) in float inAngle;
layout(location = 4) in uint inShape;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragSize;
layout(location = 2) out float fragAngle;
layout(location = 3) flat out uint fragShape;

vec4 transform(vec3 pos)
{
    vec4 tr = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    tr.y = -tr.y;
    tr.z = 0.5 * (tr.z + tr.w);
    return tr;
}

void main()
{
    gl_Position = transform(inPos);
    gl_PointSize = max(inSize, 0.0);
    fragColor = inColor;
    fragSize = max(inSize, 1.0);
    fragAngle = inAngle;
    fragShape = inShape;
}
