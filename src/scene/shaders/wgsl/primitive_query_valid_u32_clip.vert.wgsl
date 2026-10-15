// datoviz-builtin-shader: scene.primitive query_valid_u32 vertex v1

#include "common.wgsl"

struct VertexIn {
    @location(8) fragment_clip: vec2f,
    @location(0) position: vec3f,
    @location(1) identity: vec2u,
}

struct VertexOut {
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @location(1) alpha: f32,
    @builtin(position) position: vec4f,
    @location(0) @interpolate(flat) id: u32,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.clip_distance = input.fragment_clip.x;
    output.clip_include_zero = u32(input.fragment_clip.y);
    output.position = transform(input.position);
    output.id = input.identity.x;
    output.alpha = bitcast<f32>(input.identity.y);
    return output;
}
