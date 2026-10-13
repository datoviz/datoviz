#version 450

#include "scene_material.glsl"
#include "wboit.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 2) in float fragSize;
layout(location = 0) out vec4 outAccum;
layout(location = 1) out float outWeight;

#include "point_disc.glsl"

void main() {
    vec4 outColor;
    float dist = pointDiscDistance();
    float alpha = pointDiscCoverage(dist);
    if (alpha <= 0.0)
        discard;
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(applyDepthCue(linearColor.rgb, fragCue), linearColor.a * alpha);
    if (outColor.a <= 0.0)
        discard;
    outAccum = sceneWboitAccum(outColor);
    outWeight = sceneWboitWeight(outColor.a);
}
