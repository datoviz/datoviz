#ifdef DVZ_MESH_FRAGMENT_CLIP
layout(location = 8) in vec2 inFragmentClip;
layout(location = 5) out float fragClipDistance;
layout(location = 6) flat out uint fragClipIncludeZero;
void passFragmentClip()
{
    fragClipDistance = inFragmentClip.x;
    fragClipIncludeZero = uint(inFragmentClip.y);
}
#else
void passFragmentClip() {}
#endif
