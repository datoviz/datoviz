#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) cue: vec3f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let color = semantic_color_to_linear(input.color);
    return vec4f(apply_depth_cue(color.rgb, input.cue), color.a);
}
