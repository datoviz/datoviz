#version 450

vec2 p[3] = vec2[](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));

void main()
{
    gl_Position = vec4(p[gl_VertexIndex], 0, 1);
}
