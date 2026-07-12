// datoviz-builtin-shader: scene.primitive query_u32 vertex v1

#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) id: u32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) @interpolate(flat) id: u32,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = transform(input.position);
    output.id = input.id;
    return output;
}
