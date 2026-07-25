#version 450

layout(location = 0) in vec3 barycentric;
layout(location = 0) out vec4 out_color;

void main()
{
    float nearest_edge = min(barycentric.x, min(barycentric.y, barycentric.z));
    float glow = smoothstep(0.0, 0.08, nearest_edge);
    vec3 interior = 0.25 + 0.75 * pow(barycentric, vec3(0.45));
    vec3 edge = vec3(0.03, 0.90, 1.00);
    out_color = vec4(mix(edge, interior, glow), 1.0);
}
