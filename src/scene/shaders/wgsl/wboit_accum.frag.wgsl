#include "color.wgsl"
#include "wboit.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
}

struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let color = semantic_color_to_linear(input.color);
    var output: FragmentOut;
    output.accum = scene_wboit_accum(color);
    output.weight = scene_wboit_weight(color.a);
    return output;
}
