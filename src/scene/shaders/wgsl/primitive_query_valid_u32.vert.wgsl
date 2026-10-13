// datoviz-builtin-shader: scene.primitive query_valid_u32 vertex v1

#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) identity: vec2u,
}

struct VertexOut {
    @location(1) alpha: f32,
    @builtin(position) position: vec4f,
    @location(0) @interpolate(flat) id: u32,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = transform(input.position);
    output.id = input.identity.x;
    output.alpha = bitcast<f32>(input.identity.y);
    return output;
}
