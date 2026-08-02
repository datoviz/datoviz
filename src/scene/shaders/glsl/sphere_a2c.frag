#version 450

#include "common.glsl"
#include "scene_material.glsl"
#include "sphere_analytic.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragCenterView;
layout(location = 2) in float fragRadius;
layout(location = 3) in float fragSpriteRadiusPx;
layout(location = 0) out vec4 outColor;

void main()
{
    DvzSphereHit hit;
    if (!sphereIntersect(fragCenterView, fragRadius, fragSpriteRadiusPx, hit) || hit.coverage <= 0.0)
        discard;
    gl_FragDepth = hit.deviceDepth;

    vec4 surfaceWorld4 = inverse(mvp.view) * hit.positionView;
    vec3 surfaceWorld = surfaceWorld4.xyz / max(abs(surfaceWorld4.w), 1e-6);
    vec3 cameraWorld = (inverse(mvp.view) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 normalWorld = normalize(mat3(inverse(mvp.view)) * hit.normalView);
    vec4 shaded = evaluateSceneMaterial(fragColor, normalWorld, surfaceWorld, cameraWorld);
    vec3 cue = vec3(gl_FragDepth, length(cameraWorld - surfaceWorld), length(surfaceWorld));
    outColor = vec4(applyDepthCue(shaded.rgb, cue), shaded.a * hit.coverage);
}
