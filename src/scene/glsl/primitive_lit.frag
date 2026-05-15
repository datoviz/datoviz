#version 450

layout(set = 1, binding = 0) uniform PrimitiveShading {
    vec4 lightDir;
    vec4 params;
} shading;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(shading.lightDir.xyz);
    vec3 v = normalize(fragCameraPos - fragWorldPos);
    vec3 h = normalize(l + v);
    float lambert = max(dot(n, l), 0.0);
    float spec = pow(max(dot(n, h), 0.0), 32.0);
    vec3 rgb = fragColor.rgb * (shading.params.x + shading.params.y * lambert) + vec3(0.18 * spec);
    outColor = vec4(clamp(rgb, 0.0, 1.0), fragColor.a);
}
