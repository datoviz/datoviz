#include "color.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) size: f32,
}

#include "point_disc.wgsl"
@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = point_disc_distance(input.corner, input.size);
    let alpha = point_disc_coverage(dist);
    if (alpha <= 0.0) {
        discard;
    }
    let color = semantic_color_to_linear(input.color);
    if (color.a * alpha <= 0.0) {
        discard;
    }
    return vec4f(color.rgb, color.a * alpha);
}
