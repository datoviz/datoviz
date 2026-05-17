#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float dist = length(uv);
    float aa = max(fwidth(dist), 1e-6);
    float alpha = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, dist);
    if (alpha <= 0.0)
        discard;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
