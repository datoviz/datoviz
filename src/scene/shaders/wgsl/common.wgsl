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

// Scene matrices use the right-handed OpenGL clip convention (depth [-w, +w]). WebGPU keeps
// scene-clip Y and lowers only depth into the device [0, w] range.
fn scene_clip_to_device_clip(scene_clip: vec4f) -> vec4f {
    return vec4f(scene_clip.xy, 0.5 * (scene_clip.z + scene_clip.w), scene_clip.w);
}

fn transform(position: vec3f) -> vec4f {
    let scene_clip = mvp.proj * mvp.view * mvp.model * vec4f(position, 1.0);
    return scene_clip_to_device_clip(scene_clip);
}
