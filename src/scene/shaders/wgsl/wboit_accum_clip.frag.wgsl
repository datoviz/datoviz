#include "mesh_fragment_clip.wgsl"
#include "color.wgsl"
#include "wboit.wgsl"

struct FragmentIn {
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @location(0) color: vec4f,
}

struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    if (!mesh_fragment_covered(input.clip_distance, input.clip_include_zero)) { discard; }
    let color = semantic_color_to_linear(input.color);
    var output: FragmentOut;
    output.accum = scene_wboit_accum(color);
    output.weight = scene_wboit_weight(color.a);
    return output;
}
