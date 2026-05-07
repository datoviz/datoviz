#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPos, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
