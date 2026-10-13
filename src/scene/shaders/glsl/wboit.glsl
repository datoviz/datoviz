#ifndef DVZ_SCENE_WBOIT_GLSL
#define DVZ_SCENE_WBOIT_GLSL

vec4 sceneWboitAccum(vec4 linearColor)
{
    float alpha = clamp(linearColor.a, 0.0, 1.0);
    return vec4(linearColor.rgb * alpha, alpha);
}

float sceneWboitWeight(float alpha)
{
    return -log(max(1.0 - clamp(alpha, 0.0, 1.0), 1e-4));
}

#endif
