#ifndef DVZ_SCENE_OCCLUSION_GLSL
#define DVZ_SCENE_OCCLUSION_GLSL

#ifndef DVZ_SCENE_OCCLUSION_SET
#define DVZ_SCENE_OCCLUSION_SET 2
#endif

layout(set = DVZ_SCENE_OCCLUSION_SET, binding = 0) uniform sampler2D sceneOcclusionDepth;

layout(set = DVZ_SCENE_OCCLUSION_SET, binding = 2) uniform SceneOcclusionParams {
    vec4 params;
} sceneOcclusion;

float sceneOcclusionVisibility(float selfDepth)
{
    if (sceneOcclusion.params.w < 0.5) {
        return 1.0;
    }

    vec2 size = vec2(textureSize(sceneOcclusionDepth, 0));
    vec2 uv = clamp(gl_FragCoord.xy / size, vec2(0.0), vec2(1.0));
    float sceneDepth = texture(sceneOcclusionDepth, uv).r;
    if (sceneDepth >= 0.999999) {
        return 1.0;
    }

    float depthBias = sceneOcclusion.params.x;
    float softEdge = max(sceneOcclusion.params.y, 0.000001);
    float hiddenAlpha = clamp(sceneOcclusion.params.z, 0.0, 1.0);
    float delta = selfDepth - sceneDepth - depthBias;
    if (delta <= 0.0) {
        return 1.0;
    }

    float fade = smoothstep(0.0, softEdge, delta);
    return mix(1.0, hiddenAlpha, fade);
}

void applySceneOcclusionDepth(inout vec4 color, float selfDepth)
{
    color.a *= sceneOcclusionVisibility(selfDepth);
    if (color.a <= 0.0) {
        discard;
    }
}

void applySceneOcclusion(inout vec4 color)
{
    applySceneOcclusionDepth(color, gl_FragCoord.z);
}

#endif
