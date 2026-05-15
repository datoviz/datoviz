#version 450

layout(set = 1, binding = 0) uniform PrimitiveShading {
    vec4 lightDir;
    vec4 params;
} shading;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 0) out vec4 frontAccum;
layout(location = 1) out vec4 backAccum;
layout(location = 2) out vec4 depthPair;

vec4 shade()
{
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(shading.lightDir.xyz);
    vec3 v = normalize(fragCameraPos - fragWorldPos);
    vec3 h = normalize(l + v);
    float lambert = max(dot(n, l), 0.0);
    float spec = pow(max(dot(n, h), 0.0), 32.0);
    vec3 rgb = fragColor.rgb * (shading.params.x + shading.params.y * lambert) + vec3(0.18 * spec);
    return vec4(clamp(rgb, 0.0, 1.0), fragColor.a);
}

void main()
{
    vec4 c = shade();
    float a = clamp(c.a, 0.0, 1.0);
    frontAccum = vec4(0.0);
    backAccum = vec4(c.rgb * a, a);
    depthPair = vec4(gl_FragCoord.z, 1.0 - gl_FragCoord.z, 0.0, 1.0);
}
