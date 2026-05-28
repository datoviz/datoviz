#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = transform(input.position);
    output.color = input.color;
    return output;
}
