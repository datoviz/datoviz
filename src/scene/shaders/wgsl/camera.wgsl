fn dvz_inverse_mat3x3f(m: mat3x3f) -> mat3x3f {
    let c0 = cross(m[1], m[2]);
    let c1 = cross(m[2], m[0]);
    let c2 = cross(m[0], m[1]);
    let inv_det = 1.0 / dot(m[0], c0);
    return transpose(mat3x3f(c0 * inv_det, c1 * inv_det, c2 * inv_det));
}

fn dvz_affine_inverse_translation(m: mat4x4f) -> vec3f {
    let linear = mat3x3f(m[0].xyz, m[1].xyz, m[2].xyz);
    return -(dvz_inverse_mat3x3f(linear) * m[3].xyz);
}

fn camera_position_from_view() -> vec3f {
    return dvz_affine_inverse_translation(mvp.view);
}
