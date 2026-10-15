#ifdef DVZ_MESH_FRAGMENT_CLIP
layout(location = 5) in float fragClipDistance;
layout(location = 6) flat in uint fragClipIncludeZero;
void applyFragmentClip()
{
    if (!(fragClipDistance > 0.0 || (fragClipDistance == 0.0 && fragClipIncludeZero != 0u)))
        discard;
}
#else
void applyFragmentClip() {}
#endif
