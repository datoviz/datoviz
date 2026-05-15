struct MVP {
    model: mat4x4f,
    view: mat4x4f,
    proj: mat4x4f,
    time: f32,
    flags: u32,
}

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@group(0) @binding(0) var<uniform> mvp: MVP;

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    output.uv = input.uv;
    return output;
}
