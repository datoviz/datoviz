#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 2) uniform GlyphParams
{
    vec4 params;
} glyph;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragSize;
layout(location = 2) in float fragAngle;
layout(location = 3) flat in uint fragShape;
layout(location = 4) in float fragSpriteScale;
layout(location = 5) in vec4 fragTexRect;

layout(location = 0) out vec4 outColor;

#define GLYPH_ENCODING_SDF_ALPHA    1
#define GLYPH_ENCODING_MSDF_RGB     2

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPixelRange(vec2 uv)
{
    float pixelRange = max(glyph.params.x, 1.0);
    vec2 unitRange = vec2(pixelRange) / vec2(textureSize(sampler2D(tex, samp), 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(uv);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float distanceOpacity(float sd, vec2 uv)
{
    float screenPxDistance = screenPixelRange(uv) * (sd - 0.5);
    return clamp(screenPxDistance + 0.5, 0.0, 1.0);
}

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
    float sd = texel.r;
    int encoding = int(glyph.params.y + 0.5);
    if (encoding == GLYPH_ENCODING_MSDF_RGB)
        sd = median3(texel.r, texel.g, texel.b);

    float opacity = distanceOpacity(sd, uv);
    outColor = vec4(fragColor.rgb, fragColor.a * opacity);
    if (outColor.a <= 0.0)
        discard;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
