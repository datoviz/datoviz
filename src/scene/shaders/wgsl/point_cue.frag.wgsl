#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) cue: vec3f,
    @location(3) size: f32,
}

fn point_disc_distance(corner: vec2f, size: f32) -> f32 {
    let point_size = max(size, 0.0);
    let sprite_size = max(point_size + 4.0, 1.0);
    return length(corner * 0.5 * sprite_size) - 0.5 * point_size;
}

fn point_disc_coverage(dist: f32) -> f32 {
    let aa = max(fwidth(dist), 1e-6);
    return 1.0 - smoothstep(-aa, aa, dist);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = point_disc_distance(input.corner, input.size);
    let alpha = point_disc_coverage(dist);
    if (alpha <= 0.0) {
        discard;
    }
    let color = semantic_color_to_linear(input.color);
    return vec4f(apply_depth_cue(color.rgb, input.cue), color.a * alpha);
}
