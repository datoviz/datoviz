#version 450

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

layout(set = 1, binding = 0) uniform utexture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 2) uniform LabelsParams
{
    uvec4 ids;
    uvec4 params;
    vec4 floats;
    vec4 boundary_color;
    uvec4 hidden_ids[64];
    uvec4 label_lookup[65];
} labels;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

#define LABELS_FLAG_SELECTED 0x01u
#define LABELS_FLAG_BOUNDARY 0x02u

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
    uint count = min(labels.label_lookup[0].x, 64u);
    for (uint i = 1u; i <= count; i++)
    {
        if (labels.label_lookup[i].x == id)
        {
            uint rgba = labels.label_lookup[i].y;
            return vec4(
                float((rgba >> 0) & 0xffu),
                float((rgba >> 8) & 0xffu),
                float((rgba >> 16) & 0xffu),
                float((rgba >> 24) & 0xffu)) / 255.0;
        }
    }
    uint h = hashLabel(id ^ labels.params.y);
    vec3 rgb = vec3(
        float((h >> 0) & 0xffu),
        float((h >> 8) & 0xffu),
        float((h >> 16) & 0xffu)) / 255.0;
    return vec4(mix(vec3(0.18), rgb, 0.82), 0.82);
}

bool isHidden(uint id)
{
    uint count = min(labels.params.z, 256u);
    for (uint i = 0u; i < count; i++)
    {
        uint bits = labels.hidden_ids[i / 4u][i % 4u];
        if (id == bits)
            return true;
    }
    return false;
}

uint loadLabel(ivec2 coord, ivec2 size)
{
    return texelFetch(usampler2D(tex, samp), clamp(coord, ivec2(0), size - ivec2(1)), 0).r;
}

bool selectedBoundary(ivec2 coord, ivec2 size, uint selected_id)
{
    int radius = clamp(int(round(labels.floats.y)), 1, 16);
    for (int dy = -radius; dy <= radius; dy++)
    {
        for (int dx = -radius; dx <= radius; dx++)
        {
            if (loadLabel(coord + ivec2(dx, dy), size) != selected_id)
                return true;
        }
    }
    return false;
}

void main()
{
    ivec2 size = textureSize(usampler2D(tex, samp), 0);
    ivec2 coord = clamp(ivec2(floor(fragUV * vec2(size))), ivec2(0), size - ivec2(1));
    uint id = loadLabel(coord, size);
    if (id == labels.ids.x || isHidden(id))
        discard;
    outColor = labelColor(id);
    bool selected = (labels.params.x & LABELS_FLAG_SELECTED) != 0u && id == labels.ids.y;
    if (selected)
    {
        bool boundary = (labels.params.x & LABELS_FLAG_BOUNDARY) != 0u &&
                        selectedBoundary(coord, size, labels.ids.y);
        if (boundary)
            outColor = labels.boundary_color;
        else
            outColor = mix(outColor, labels.boundary_color, 0.35);
    }
    outColor.a *= labels.floats.x;
#ifdef DVZ_SCENE_OCCLUSION
    applySceneOcclusion(outColor);
#endif
}
