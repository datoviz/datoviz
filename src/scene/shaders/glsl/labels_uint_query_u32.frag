#version 450

layout(set = 1, binding = 0) uniform utexture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 2) uniform LabelsParams
{
    uvec4 ids;
    uvec4 params;
    vec4 floats;
    vec4 boundary_color;
    uvec4 hidden_ids[64];
} labels;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out uint outId;

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

void main()
{
    ivec2 size = textureSize(usampler2D(tex, samp), 0);
    ivec2 coord = clamp(ivec2(floor(fragUV * vec2(size))), ivec2(0), size - ivec2(1));
    uint id = texelFetch(usampler2D(tex, samp), coord, 0).r;
    if (id == labels.ids.x || isHidden(id))
        discard;
    outId = id + 1u;
}
