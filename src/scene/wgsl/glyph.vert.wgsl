#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
    @location(2) color: vec4f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
