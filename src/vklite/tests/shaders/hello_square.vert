#version 450

layout(location = 0) out vec2 uv;

const vec2 positions[6] = vec2[](
    vec2(-.5, -.5),
    vec2(-.5, +.5),
    vec2(+.5, -.5),

    vec2(+.5, +.5),
    vec2(+.5, -.5),
    vec2(-.5, +.5)
);

void main()
{
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    uv = pos + .5;
}
