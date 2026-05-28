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
