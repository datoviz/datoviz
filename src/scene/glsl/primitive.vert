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

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 tr = mvp.proj * mvp.view * mvp.model * vec4(inPos, 1.0);
    /* OpenGL -> Vulkan NDC: flip Y, remap depth from [-1,1] to [0,1]. */
    tr.y = -tr.y;
    tr.z = 0.5 * (tr.z + tr.w);
    gl_Position = tr;
    fragColor = inColor;
}
