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
layout(location = 0) out vec4 outNormal;

vec4 projectDepth(vec4 viewPos)
{
    vec4 clip = mvp.proj * viewPos;
    clip.y = -clip.y;
    clip.z = 0.5 * (clip.z + clip.w);
    return clip;
}

float coverageThreshold(vec2 fragCoord)
{
    return fract(52.9829189 * fract(0.06711056 * fragCoord.x + 0.00583715 * fragCoord.y));
}

void main()
{
    vec2 coord = (2.0 * gl_PointCoord - 1.0) * fragSpriteScale;
    coord.y = -coord.y;
    float dist = length(coord);
    float coverage = clamp((1.0 - dist) / max(fwidth(dist), 1e-6) + 0.5, 0.0, 1.0);
    if (coverage <= coverageThreshold(gl_FragCoord.xy))
        discard;

    vec2 surfaceCoord = dist > 1.0 ? coord / max(dist, 1e-6) : coord;
    float edge = 1.0 - dot(surfaceCoord, surfaceCoord);
    float nz = sqrt(max(edge, 0.0));
    vec3 normalView = normalize(vec3(surfaceCoord, nz));
    vec4 surfaceView = fragCenterView + vec4(normalView * fragRadius, 0.0);
    vec4 depthClip = projectDepth(surfaceView);
    gl_FragDepth = clamp(depthClip.z / max(abs(depthClip.w), 1e-6), 0.0, 1.0);

    vec3 normalWorld = normalize(mat3(inverse(mvp.view)) * normalView);
    outNormal = vec4(normalWorld * 0.5 + 0.5, length(surfaceView.xyz)) + fragColor * 0.0;
}
