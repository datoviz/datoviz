#include "common.wgsl"

struct VertexIn {
    @location(8) fragment_clip: vec2f,
    @location(0) position: vec3f,
    @location(1) color: vec4f,
}

struct VertexOut {
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.clip_distance = input.fragment_clip.x;
    output.clip_include_zero = u32(input.fragment_clip.y);
    output.position = transform(input.position);
    output.color = input.color;
    return output;
}
