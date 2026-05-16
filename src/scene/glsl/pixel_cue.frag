#version 450

#include "scene_material.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(applyDepthCue(fragColor.rgb, fragCue), fragColor.a);
}
