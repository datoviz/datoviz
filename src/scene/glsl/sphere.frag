#version 450

#include "scene_material.glsl"

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragCenterView;
layout(location = 2) in float fragRadius;
layout(location = 0) out vec4 outColor;

vec4 projectDepth(vec4 viewPos)
{
    vec4 clip = mvp.proj * viewPos;
    clip.y = -clip.y;
    clip.z = 0.5 * (clip.z + clip.w);
    return clip;
}

void main()
{
    vec2 coord = 2.0 * gl_PointCoord - 1.0;
    coord.y = -coord.y;
    float r2 = dot(coord, coord);
    float edge = 1.0 - r2;
    float aa = max(fwidth(r2), 1e-6);
    float coverage = smoothstep(0.0, aa, edge);
    if (coverage <= 0.0)
        discard;

    float nz = sqrt(max(edge, 0.0));
    vec3 normalView = normalize(vec3(coord, nz));
    vec4 surfaceView = fragCenterView + vec4(normalView * fragRadius, 0.0);
    vec4 depthClip = projectDepth(surfaceView);
    gl_FragDepth = clamp(depthClip.z / max(abs(depthClip.w), 1e-6), 0.0, 1.0);

    vec4 surfaceWorld4 = inverse(mvp.view) * surfaceView;
    vec3 surfaceWorld = surfaceWorld4.xyz / max(abs(surfaceWorld4.w), 1e-6);
    vec3 cameraWorld = (inverse(mvp.view) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 normalWorld = normalize(mat3(inverse(mvp.view)) * normalView);
    vec3 l = normalize(material.lightDir.xyz);
    vec3 v = normalize(cameraWorld - surfaceWorld);
    vec3 h = normalize(l + v);
    float lambert = max(dot(normalWorld, l), 0.0);
    float spec = pow(max(dot(normalWorld, h), 0.0), max(material.params.w, 1.0));
    vec3 rgb = fragColor.rgb * (material.params.x + material.params.y * lambert) +
               vec3(material.params.z * spec);
    vec3 cue = vec3(gl_FragDepth, length(cameraWorld - surfaceWorld), length(surfaceWorld));
    outColor = vec4(applyDepthCue(clamp(rgb, 0.0, 1.0), cue), fragColor.a * coverage);
}
