#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 view_position;
layout(location = 0) out vec4 out_color;

void main()
{
    vec3 n = normalize(normal);
    vec3 l = normalize(vec3(2.0, 2.0, 0.0) - view_position);
    vec3 v = normalize(-view_position);
    vec3 h = normalize(l + v);
    float diffuse = max(dot(n, l), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), 32.0) : 0.0;
    vec3 albedo = texture(tex, uv).rgb;
    vec3 color = albedo * (0.16 + 0.84 * diffuse) + vec3(specular * 0.35);
    out_color = vec4(color, 1.0);
}
