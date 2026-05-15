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
layout(location = 2) in vec3 inNormal;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragCameraPos;

vec4 transform(vec3 pos)
{
    vec4 tr = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    tr.y = -tr.y;
    tr.z = 0.5 * (tr.z + tr.w);
    return tr;
}

void main()
{
    vec4 world = mvp.model * vec4(inPos, 1.0);
    gl_Position = transform(inPos);
    fragColor = inColor;
    fragWorldPos = world.xyz;
    fragCameraPos = (inverse(mvp.view) * vec4(0, 0, 0, 1)).xyz;
    fragNormal = transpose(inverse(mat3(mvp.model))) * inNormal;
}
