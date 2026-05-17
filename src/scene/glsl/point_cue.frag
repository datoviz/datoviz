#version 450

#include "scene_material.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragCue;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float dist = length(uv);
    float aa = max(fwidth(dist), 1e-6);
    float alpha = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, dist);
    if (alpha <= 0.0)
        discard;
    outColor = vec4(applyDepthCue(fragColor.rgb, fragCue), fragColor.a * alpha);
}
