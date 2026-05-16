#version 450

#include "scene_material.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in float fragDepth;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

void main()
{
    vec3 n = normalize(fragNormal);
    if (!gl_FrontFacing)
        n = -n;
    vec3 l = normalize(material.lightDir.xyz);
    vec3 v = normalize(fragCameraPos - fragWorldPos);
    vec3 h = normalize(l + v);
    float lambert = max(dot(n, l), 0.0);
    float spec = pow(max(dot(n, h), 0.0), 32.0);
    vec3 rgb = fragColor.rgb * (material.params.x + material.params.y * lambert) + vec3(0.18 * spec);
    float a = clamp(fragColor.a, 0.0, 1.0);
    vec3 lit = applyDepthCue(clamp(rgb, 0.0, 1.0), fragDepth);
    outAccum = vec4(lit * a, a);
    outWeight = -log(max(1.0 - a, 1e-4));
}
