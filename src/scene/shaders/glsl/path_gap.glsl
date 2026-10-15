// Endpoint masks are evaluated in the same physical-pixel plane as stroke extrusion.
// Zero radius bypasses projection entirely; hidden depth endpoints contribute no disk.
#ifdef DVZ_PATH_GAP_VERTEX
vec3 dvz_path_endpoint_delta(vec2 pixel, vec4 endpoint)
{
    if (endpoint.w <= 0.0)
        return vec3(0.0);
    vec4 clip = transform(endpoint.xyz);
    if (clip.w <= 1e-6 || clip.z < 0.0 || clip.z > clip.w)
        return vec3(0.0);
    return vec3(pixel - deviceClipToTopLeftPixel(clip), endpoint.w);
}
#else
float dvz_path_endpoint_coverage(vec3 start, vec3 end)
{
    float coverage = 1.0;
    if (start.z > 0.0)
        coverage = min(coverage, smoothstep(0.0, 1.0, length(start.xy) - start.z));
    if (end.z > 0.0)
        coverage = min(coverage, smoothstep(0.0, 1.0, length(end.xy) - end.z));
    return coverage;
}
#endif
