#ifndef DVZ_SPHERE_ANALYTIC_GLSL
#define DVZ_SPHERE_ANALYTIC_GLSL

struct DvzSphereHit
{
    vec4 positionView;
    vec3 normalView;
    float linearDepth;
    float deviceDepth;
    float coverage;
};

vec4 sphereProjectDepth(vec4 viewPosition)
{
    return sceneClipToDeviceClip(mvp.proj * viewPosition);
}

// Build the eye ray through one fragment. `ndc` is the fragment's own position, interpolated
// across the impostor quad, so the ray never depends on the proxy's size or on where the
// projected sphere centre landed.
void sphereFragmentRay(vec2 ndc, out vec3 rayOrigin, out vec3 rayDirection)
{
    mat4 invProj = inverse(mvp.proj);
    vec4 nearHomogeneous = invProj * vec4(ndc, -1.0, 1.0);
    vec4 farHomogeneous = invProj * vec4(ndc, 1.0, 1.0);
    vec3 nearView = nearHomogeneous.xyz / max(abs(nearHomogeneous.w), 1e-6);
    vec3 farView = farHomogeneous.xyz / max(abs(farHomogeneous.w), 1e-6);

    bool ortho = abs(mvp.proj[3][3]) > 0.5;
    rayOrigin = ortho ? nearView : vec3(0.0);
    rayDirection = normalize(ortho ? farView - nearView : nearView);
}

bool sphereIntersect(vec4 centerView, float radius, vec2 ndc, out DvzSphereHit hit)
{
    vec3 rayOrigin = vec3(0.0);
    vec3 rayDirection = vec3(0.0, 0.0, -1.0);
    sphereFragmentRay(ndc, rayOrigin, rayDirection);

    vec3 oc = rayOrigin - centerView.xyz;
    float b = dot(oc, rayDirection);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - c;
    float transition = max(fwidth(discriminant), 1e-8);
    hit.coverage = clamp(discriminant / transition + 0.5, 0.0, 1.0);
    if (discriminant < 0.0)
        return false;

    float root = sqrt(max(discriminant, 0.0));
    float nearDistance = -b - root;
    float farDistance = -b + root;
    float distance = nearDistance > 1e-6 ? nearDistance : farDistance;
    if (distance <= 1e-6)
        return false;

    hit.positionView = vec4(rayOrigin + distance * rayDirection, 1.0);
    hit.normalView = normalize(hit.positionView.xyz - centerView.xyz);
    hit.linearDepth = max(-hit.positionView.z, 0.0);
    vec4 depthClip = sphereProjectDepth(hit.positionView);
    if (depthClip.w <= 1e-6)
        return false;
    hit.deviceDepth = depthClip.z / depthClip.w;
    if (hit.deviceDepth < 0.0 || hit.deviceDepth > 1.0)
        return false;
    return true;
}

#endif
