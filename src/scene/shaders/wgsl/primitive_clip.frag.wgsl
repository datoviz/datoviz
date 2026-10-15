#include "mesh_fragment_clip.wgsl"
#include "color.wgsl"

struct FragmentIn {
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @location(0) color: vec4f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    if (!mesh_fragment_covered(input.clip_distance, input.clip_include_zero)) { discard; }
    return semantic_color_to_linear(input.color);
}
