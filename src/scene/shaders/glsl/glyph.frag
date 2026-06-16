#version 450

#include "color.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 2) uniform GlyphParams
{
    vec4 params;
} glyph;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

#define GLYPH_ENCODING_SDF_ALPHA    1
#define GLYPH_ENCODING_MSDF_RGB     2

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPixelRange()
{
    float pixelRange = max(glyph.params.x, 1.0);
    vec2 unitRange = vec2(pixelRange) / vec2(textureSize(sampler2D(tex, samp), 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(fragUV);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float distanceOpacity(float sd)
{
    float screenPxDistance = screenPixelRange() * (sd - 0.5);
    return clamp(screenPxDistance + 0.5, 0.0, 1.0);
}

void main()
{
    vec4 texel = texture(sampler2D(tex, samp), fragUV);
    float opacity = texel.a;
    int encoding = int(glyph.params.y + 0.5);
    if (encoding == GLYPH_ENCODING_MSDF_RGB)
    {
        /* The generated atlas is MTSDF: RGB carries sharp MSDF, alpha guards sign artifacts. */
        float msdf = median3(texel.r, texel.g, texel.b);
        float sdf = texel.a;
        float sd = msdf;
        if ((msdf - 0.5) * (sdf - 0.5) < 0.0)
            sd = sdf;
        opacity = distanceOpacity(sd);
    }
    else if (encoding == GLYPH_ENCODING_SDF_ALPHA)
    {
        opacity = distanceOpacity(texel.a);
    }
    vec4 linearColor = semanticColorToLinear(fragColor);
    outColor = vec4(linearColor.rgb, linearColor.a * opacity);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
