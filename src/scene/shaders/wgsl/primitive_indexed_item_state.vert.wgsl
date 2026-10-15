#include "common.wgsl"

#include "item_state_style.wgsl"

@group(1) @binding(2) var<storage, read> item_state_words: array<u32>;

struct VertexIn {
    @location(7) state_index: u32,
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
    output.color = apply_item_state_color(input.color, item_state_words[input.state_index]);
    return output;
}
