#ifndef DVZ_SPHERE_VERTEX_GLSL
#define DVZ_SPHERE_VERTEX_GLSL

// Sphere impostors are drawn as instanced screen-space quads rather than native point sprites.
// A point sprite is sized by gl_PointSize, which the device clamps to
// VkPhysicalDeviceLimits::pointSizeRange, and a point primitive is clipped by its centre vertex;
// either limit crops a sphere that is large on screen or whose centre leaves the frustum. A quad
// carries no such limit, so the silhouette stays correct at any camera distance or viewport size.
//
// The quad spans the exact NDC bounds of the silhouette. Under perspective the silhouette of a
// sphere is an ellipse that is NOT centred on the projected sphere centre, so the bounds come
// from the per-axis tangent extrema rather than from a symmetric radius around that centre.

// Unit-quad corner for one of the six triangle-list vertices.
vec2 sphereQuadCorner(int vertexIndex)
{
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
        vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0));
    return corners[clamp(vertexIndex, 0, 5)];
}

// Exact NDC bounds of the sphere silhouette, in the unflipped convention that mvp.proj produces.
// Returns false when the eye lies inside or on the sphere: no tangent cone exists there and the
// silhouette can cover the whole viewport, so the caller falls back to a full-viewport quad.
bool sphereSilhouetteNdc(vec3 centerView, float radius, out vec2 lower, out vec2 upper)
{
    lower = vec2(-1.0);
    upper = vec2(1.0);

    vec2 scale = vec2(mvp.proj[0][0], mvp.proj[1][1]);
    vec2 depthTerm = vec2(mvp.proj[2][0], mvp.proj[2][1]);
    vec2 constTerm = vec2(mvp.proj[3][0], mvp.proj[3][1]);
    if (abs(mvp.proj[3][3]) > 0.5)
    {
        // Orthographic: the silhouette is a circle centred on the projected centre.
        vec2 center = scale * centerView.xy + depthTerm * centerView.z + constTerm;
        vec2 extent = (abs(scale) + abs(depthTerm)) * radius;
        lower = center - extent;
        upper = center + extent;
        return true;
    }

    float z = -centerView.z;
    if (z <= radius * (1.0 + 1e-4))
        return false;

    // With w = -z, ndc.x expands to proj[0][0] * (x / z) - proj[2][0] + proj[3][0] / z, so the
    // projection can carry two lateral offsets beyond the plain scale. Both are in use: query
    // recentring shifts proj[2][0..1] to move a picked point onto its scratch pixel, and the
    // interaction hit test shifts proj[3][0..1]. Fold the depth-dependent proj[3] term into the
    // centre, which moves the tangent construction to the pole of that map, and apply the
    // constant proj[2] term as a rigid translation afterwards. The bounds stay exact for both.
    vec2 safeScale = vec2(
        abs(scale.x) > 1e-6 ? scale.x : 1e-6, abs(scale.y) > 1e-6 ? scale.y : 1e-6);
    vec2 center = centerView.xy + constTerm / safeScale;

    // Tangent lines from the pole to the sphere, per axis. `slope` is the view-space x/z (resp.
    // y/z) ratio, which the projection then maps to NDC by a plain scale.
    float denom = max(z * z - radius * radius, 1e-12);
    vec2 tangent = sqrt(max(center * center + z * z - radius * radius, vec2(0.0)));
    vec2 slope0 = (center * z - radius * tangent) / denom;
    vec2 slope1 = (center * z + radius * tangent) / denom;
    vec2 ndc0 = scale * slope0 - depthTerm;
    vec2 ndc1 = scale * slope1 - depthTerm;
    lower = min(ndc0, ndc1);
    upper = max(ndc0, ndc1);
    return true;
}

// Place one impostor-quad vertex. `clipPosition` is a screen-space rectangle at a fixed depth:
// every sphere fragment writes gl_FragDepth from its own ray hit, so the proxy's rasterized depth
// is never read, and keeping it inside the view volume stops the near and far planes from
// clipping a proxy whose sphere is still partly visible. `ndc` is this fragment's position in the
// unflipped NDC convention that sphereIntersect() unprojects.
void sphereQuadVertex(
    vec3 centerView, float radius, int vertexIndex, out vec4 clipPosition, out vec2 ndc)
{
    vec2 corner = sphereQuadCorner(vertexIndex);
    vec2 lower, upper;
    if (sphereSilhouetteNdc(centerView, radius, lower, upper))
    {
        // Two pixels of padding keep the analytic silhouette antialiasing inside the quad.
        vec2 pad = 4.0 / max(viewport.rect.zw, vec2(1.0));
        lower = clamp(lower - pad, vec2(-1.0), vec2(1.0));
        upper = clamp(upper + pad, vec2(-1.0), vec2(1.0));
        ndc = mix(lower, upper, 0.5 * (corner + 1.0));
    }
    else
    {
        ndc = vec2(corner.x, -corner.y);
    }
    // common.glsl transform() negates y on the way to Vulkan clip space; match it here.
    clipPosition = vec4(ndc.x, -ndc.y, 0.5, 1.0);
}

#endif
