#include "mesh_fragment_clip.wgsl"
// datoviz-builtin-shader: scene.primitive query_depth_rg32 fragment v1

struct FragmentIn {
    @builtin(position) position: vec4<f32>,
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @location(1) alpha: f32,
    @location(0) @interpolate(flat) id: u32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec2<u32> {
    if (!mesh_fragment_covered(input.clip_distance, input.clip_include_zero)) { discard; }
    if (!(input.alpha > 0.0) || input.alpha > 3.402823466e38) { discard; }
    return vec2<u32>(input.id, bitcast<u32>(input.position.z));
}
