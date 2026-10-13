#ifndef DVZ_MESH_SURFACE_GLSL
#define DVZ_MESH_SURFACE_GLSL

#include "scene_material.glsl"
#include "scene_alpha.glsl"

layout(set = 1, binding = 1) uniform texture2D tex;
layout(set = 1, binding = 2) uniform sampler samp;
layout(set = 1, binding = 3) uniform TextureParams {
    vec4 params;
} textureParams;

vec4 evaluateMeshSurface(
    vec4 color, vec3 normal, vec2 uv, float depth, vec3 worldPos, vec3 cameraPos)
{
    vec4 texel = sampledTextureColorToLinear(
        texture(sampler2D(tex, samp), uv), textureParams.params);
    vec4 base = texel * semanticColorToLinear(color);
    vec4 shaded = evaluateSceneMaterialLinearItem(base, normal, worldPos, cameraPos);
    vec3 cue = vec3(depth, length(cameraPos - worldPos), length(worldPos));
    return vec4(applyDepthCue(shaded.rgb, cue), shaded.a);
}

#endif
