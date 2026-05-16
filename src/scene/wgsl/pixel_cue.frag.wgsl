#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) depth: f32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    return vec4f(apply_depth_cue(input.color.rgb, input.depth), input.color.a);
}
