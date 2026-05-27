#if defined(DVZ_VOLUME_LABEL_UINT_QUERY) || defined(DVZ_VOLUME_LABEL_SINT_QUERY)
ivec3 label_coord(vec3 uvw)
{
    vec3 tuv = texture_uvw(uvw);
#if defined(DVZ_VOLUME_LABEL_UINT_QUERY)
    ivec3 size = textureSize(usampler3D(tex, samp), 0);
#else
    ivec3 size = textureSize(isampler3D(tex, samp), 0);
#endif
    return clamp(ivec3(floor(tuv * vec3(size))), ivec3(0), size - ivec3(1));
}
#endif
