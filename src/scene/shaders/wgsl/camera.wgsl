fn camera_position_from_view() -> vec3f {
    return dvz_affine_inverse_translation(mvp.view);
}
