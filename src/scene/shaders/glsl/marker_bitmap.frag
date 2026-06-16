#version 450

#include "color.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragSize;
layout(location = 2) in float fragAngle;
layout(location = 3) flat in uint fragShape;
layout(location = 4) in float fragSpriteScale;
layout(location = 5) in vec4 fragTexRect;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 local = (gl_PointCoord * 2.0 - 1.0) * max(fragSpriteScale, 1.0);
    float c = cos(fragAngle);
    float s = sin(fragAngle);
    vec2 p = mat2(c, -s, s, c) * local;
    if (max(abs(p.x), abs(p.y)) > 1.0)
        discard;

    vec2 markerUV = p * 0.5 + 0.5;
    vec2 uv = mix(fragTexRect.xy, fragTexRect.zw, markerUV);
    vec4 texel = texture(sampler2D(tex, samp), uv);
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(linearColor.rgb * texel.rgb, linearColor.a * texel.a);
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
