#include "wboit.wgsl"

#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) cue: vec3f,
    @location(3) size: f32,
}

#include "point_disc.wgsl"
#include "point_style_surface.wgsl"

struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let dist = point_disc_distance(input.corner, input.size);
    let color = point_style_surface(input.color, dist);
    let surface = vec4f(apply_depth_cue(color.rgb, input.cue), color.a);
    if (surface.a <= 0.0) {
        discard;
    }
    var output: FragmentOut;
    output.accum = scene_wboit_accum(surface);
    output.weight = scene_wboit_weight(surface.a);
    return output;
}
