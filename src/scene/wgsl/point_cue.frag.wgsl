#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) cue: vec3f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    if (dot(input.corner, input.corner) > 1.0) {
        discard;
    }
    return vec4f(apply_depth_cue(input.color.rgb, input.cue), input.color.a);
}
