#ifndef DVZ_SPHERE_VERTEX_GLSL
#define DVZ_SPHERE_VERTEX_GLSL

// Return a conservative, isotropic point-sprite radius for the projected sphere. In perspective
// projections the silhouette is not centered on the projected sphere center, so both tangent
// extrema are measured relative to that center before choosing the enclosing square.
float sphereProjectedRadiusPx(vec3 centerView, float radius)
{
    vec2 viewportSize = max(viewport.rect.zw, vec2(1.0));
    bool ortho = abs(mvp.proj[3][3]) > 0.5;
    if (ortho)
    {
        vec2 radiusNdc = radius * abs(vec2(mvp.proj[0][0], mvp.proj[1][1]));
        return 0.5 * max(radiusNdc.x * viewportSize.x, radiusNdc.y * viewportSize.y);
    }

    float z = -centerView.z;
    if (z <= radius + 1e-6)
        return 0.5 * max(viewportSize.x, viewportSize.y);

    float denom = max(z * z - radius * radius, 1e-12);
    vec2 centerSlope = centerView.xy / z;
    vec2 tangentDistance = sqrt(max(centerView.xy * centerView.xy + z * z - radius * radius, vec2(0.0)));
    vec2 slope0 = (centerView.xy * z - radius * tangentDistance) / denom;
    vec2 slope1 = (centerView.xy * z + radius * tangentDistance) / denom;
    vec2 slopeRadius = max(abs(slope0 - centerSlope), abs(slope1 - centerSlope));
    vec2 radiusNdc = abs(vec2(mvp.proj[0][0], mvp.proj[1][1])) * slopeRadius;
    return 0.5 * max(radiusNdc.x * viewportSize.x, radiusNdc.y * viewportSize.y);
}

#endif
