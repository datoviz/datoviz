#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) coord: vec2f,
    @location(2) normal: vec3f,
    @location(3) world_position: vec3f,
    @location(4) camera_position: vec3f,
    @location(5) depth: f32,
}

fn coverage_threshold(position: vec4f) -> f32 {
    return fract(52.9829189 * fract(0.06711056 * position.x + 0.00583715 * position.y));
}

@fragment
fn main(input: FragmentIn, @builtin(position) frag_position: vec4f) -> @location(0) vec4f {
    let dist = length(input.coord);
    let coverage = clamp((1.0 - dist) / max(fwidth(dist), 1e-6) + 0.5, 0.0, 1.0);
    if (coverage <= coverage_threshold(frag_position)) {
        discard;
    }

    let shaded = evaluate_scene_material(
        input.color, normalize(input.normal), input.world_position, input.camera_position);
    let cue = vec3f(
        input.depth, length(input.camera_position - input.world_position),
        length(input.world_position));
    let color = vec4f(apply_depth_cue(shaded.rgb, cue), shaded.a);
    if (color.a <= 0.0) {
        discard;
    }
    return color;
}
