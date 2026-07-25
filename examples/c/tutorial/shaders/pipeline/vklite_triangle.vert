#version 450

layout(location = 0) out vec3 barycentric;

const vec2 positions[3] = vec2[3](
    vec2( 0.0, -0.65),
    vec2( 0.65, 0.65),
    vec2(-0.65, 0.65));

const vec3 corners[3] = vec3[3](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0));

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    barycentric = corners[gl_VertexIndex];
}
