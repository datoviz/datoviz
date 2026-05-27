#if defined(DVZ_VOLUME_LABEL_UINT) || defined(DVZ_VOLUME_LABEL_SINT)
layout(std430, set = 1, binding = 5) readonly buffer VolumeLabelLookup {
    uvec4 header;   // x=count, y=flags, z=fallback_seed, w=reserved
    uvec4 entries[]; // x=key_bits, y=rgba8, z=metadata_index, w=flags
} labelLut;

uint hashLabel(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

vec4 fallback_label_color(uint id)
{
    uint h = hashLabel(id ^ 0x9e3779b9u);
    vec3 rgb = vec3(
        float((h >> 0) & 0xffu),
        float((h >> 8) & 0xffu),
        float((h >> 16) & 0xffu)) / 255.0;
    return vec4(mix(vec3(0.18), rgb, 0.82), 0.82);
}

vec4 unpack_label_rgba(uint rgba)
{
    return vec4(
        float((rgba >> 0) & 0xffu),
        float((rgba >> 8) & 0xffu),
        float((rgba >> 16) & 0xffu),
        float((rgba >> 24) & 0xffu)) / 255.0;
}

bool label_lookup_color(uint key, out vec4 color)
{
    uint count = labelLut.header.x;
    uint lo = 0u;
    uint hi = count;
    while (lo < hi) {
        uint mid = lo + ((hi - lo) >> 1);
        uint entry_key = labelLut.entries[mid].x;
        if (entry_key == key) {
            color = unpack_label_rgba(labelLut.entries[mid].y);
            return true;
        }
        if (entry_key < key) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    color = vec4(0.0);
    return false;
}

ivec3 label_coord(vec3 uvw)
{
    vec3 tuv = texture_uvw(uvw);
#if defined(DVZ_VOLUME_LABEL_UINT)
    ivec3 size = textureSize(usampler3D(tex, samp), 0);
#else
    ivec3 size = textureSize(isampler3D(tex, samp), 0);
#endif
    return clamp(ivec3(floor(tuv * vec3(size))), ivec3(0), size - ivec3(1));
}

vec4 label_palette_color(uint key)
{
    vec4 lookup_color = vec4(0.0);
    if (label_lookup_color(key, lookup_color)) {
        return lookup_color;
    }
    ivec2 size = textureSize(sampler2D(transferTex, samp), 0);
    if (key < uint(size.x)) {
        return texelFetch(sampler2D(transferTex, samp), ivec2(int(key), 0), 0);
    }
    return fallback_label_color(key);
}
#endif
