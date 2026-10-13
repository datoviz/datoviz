#ifndef DVZ_SCENE_ALPHA_GLSL
#define DVZ_SCENE_ALPHA_GLSL

// SceneMaterial.model: x=model, y=opacity, z=alpha cutoff, w=DvzAlphaMode.
bool sceneAlphaValid(float alpha, vec4 model)
{
    return alpha > 0.0 && (int(model.w + 0.5) != 4 || alpha >= model.z);
}

#endif
