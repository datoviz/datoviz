// datoviz-builtin-shader: scene.primitive lit vertex v1

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

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) normal: vec3f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
}

@group(0) @binding(0) var<uniform> mvp: MVP;
@group(0) @binding(1) var<uniform> viewport: Viewport;

fn transform(position: vec3f) -> vec4f {
    return mvp.proj * mvp.view * mvp.model * vec4f(position, 1.0);
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    let world = mvp.model * vec4f(input.position, 1.0);
    let clip = transform(input.position);
    output.position = clip;
    output.color = input.color;
    output.world_position = world.xyz;
    output.camera_position = vec3f(0.0, 0.0, 3.0);
    output.normal = mat3x3f(
        mvp.model[0].xyz,
        mvp.model[1].xyz,
        mvp.model[2].xyz
    ) * input.normal;
    output.depth = clip.z / max(abs(clip.w), 1e-6);
    return output;
}
