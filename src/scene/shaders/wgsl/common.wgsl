// Shared transform pipeline for builtin scene vertex shaders.
//
// Bind group layout (group 0):
//   binding 0: MVP { model, view, proj, time, flags }
//   binding 1: Viewport { x, y, width, height }

struct MVP {
    model: mat4x4f,
    view: mat4x4f,
    proj: mat4x4f,
    time: f32,
    flags: u32,
}

struct Viewport {
    rect: vec4f,
}

@group(0) @binding(0) var<uniform> mvp: MVP;
@group(0) @binding(1) var<uniform> viewport: Viewport;

fn transform(position: vec3f) -> vec4f {
    return mvp.proj * mvp.view * mvp.model * vec4f(position, 1.0);
}

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
