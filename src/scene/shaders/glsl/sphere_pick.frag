#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragCenterView;
layout(location = 2) in float fragRadius;
layout(location = 3) in float fragSpriteScale;
layout(location = 0) out vec4 outColor;

vec4 projectDepth(vec4 viewPos)
{
    vec4 clip = mvp.proj * viewPos;
    clip.y = -clip.y;
    clip.z = 0.5 * (clip.z + clip.w);
    return clip;
}

bool raycastSphere(vec2 coord, out vec4 surfaceView)
{
    bool ortho = abs(mvp.proj[3][3]) > 0.5;
    vec3 ro = vec3(0.0);
    vec3 rd = vec3(0.0, 0.0, -1.0);
    vec3 planePoint =
        fragCenterView.xyz + vec3(coord.x * fragRadius, coord.y * fragRadius, 0.0);
    if (ortho)
        ro = planePoint + vec3(0.0, 0.0, 2.0 * fragRadius);
    else
        rd = normalize(planePoint);

    vec3 oc = ro - fragCenterView.xyz;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - fragRadius * fragRadius;
    float h = b * b - c;
    if (h < 0.0)
        return false;
    float t = -b - sqrt(h);
    if (t <= 0.0)
        return false;
    surfaceView = vec4(ro + t * rd, 1.0);
    return true;
}

void main()
{
    vec2 coord = (2.0 * gl_PointCoord - 1.0) * fragSpriteScale;
    coord.y = -coord.y;
    float dist2 = dot(coord, coord);
    if (dist2 > 1.0)
        discard;

    vec4 surfaceView = vec4(0.0);
    if (!raycastSphere(coord, surfaceView))
        discard;

    vec4 depthClip = projectDepth(surfaceView);
    gl_FragDepth = clamp(depthClip.z / max(abs(depthClip.w), 1e-6), 0.0, 1.0);
    outColor = fragColor;
}
