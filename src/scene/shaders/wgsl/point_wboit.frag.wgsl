#include "wboit.wgsl"

#include "color.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) size: f32,
}

#include "point_disc.wgsl"
struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let dist = point_disc_distance(input.corner, input.size);
    let alpha = point_disc_coverage(dist);
    if (alpha <= 0.0) {
        discard;
    }
    let color = semantic_color_to_linear(input.color);
    let surface = vec4f(color.rgb, color.a * alpha);
    if (surface.a <= 0.0) {
        discard;
    }
    var output: FragmentOut;
    output.accum = scene_wboit_accum(surface);
    output.weight = scene_wboit_weight(surface.a);
    return output;
}
