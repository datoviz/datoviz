#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform utexture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

uint hashLabel(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

vec4 labelColor(uint id)
{
    uint h = hashLabel(id);
    vec3 rgb = vec3(
        float((h >> 0) & 0xffu),
        float((h >> 8) & 0xffu),
        float((h >> 16) & 0xffu)) / 255.0;
    return vec4(mix(vec3(0.18), rgb, 0.82), 0.82);
}

void main()
{
    ivec2 size = textureSize(usampler2D(tex, samp), 0);
    ivec2 coord = clamp(ivec2(floor(fragUV * vec2(size))), ivec2(0), size - ivec2(1));
    uint id = texelFetch(usampler2D(tex, samp), coord, 0).r;
    if (id == 0u)
        discard;
    outColor = labelColor(id);
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
