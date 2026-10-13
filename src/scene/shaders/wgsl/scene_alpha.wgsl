// SceneMaterial.model: x=model, y=opacity, z=alpha cutoff, w=DvzAlphaMode.
fn scene_alpha_valid(alpha: f32, model: vec4f) -> bool {
    return alpha > 0.0 && (i32(model.w + 0.5) != 4 || alpha >= model.z);
}
