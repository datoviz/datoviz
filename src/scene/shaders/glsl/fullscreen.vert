#version 450

vec2 p[3] = vec2[](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));

layout(location = 0) out vec2 localUv;

void main()
{
    vec2 position = p[gl_VertexIndex];
    localUv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0, 1);
}
