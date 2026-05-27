#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in float fragDepth;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 texel = texture(sampler2D(tex, samp), fragUV);
    vec4 base = texel * fragColor;
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(vec3(0.35, 0.55, 0.75));
    float diffuse = max(dot(n, l), 0.0);
    vec3 rgb = base.rgb * (0.28 + 0.72 * diffuse);
    outColor = vec4(clamp(rgb, 0.0, 1.0), base.a);
    if (outColor.a <= 0.0) {
        discard;
    }
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusionDepth(outColor, fragDepth);
#endif
}
