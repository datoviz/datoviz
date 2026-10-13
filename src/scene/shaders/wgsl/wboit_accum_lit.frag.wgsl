#include "scene_material.wgsl"
#include "wboit.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
    @builtin(front_facing) front_facing: bool,
}

struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let normal = select(-normalize(input.normal), normalize(input.normal), input.front_facing);
    let shaded = evaluate_scene_material(
        input.color, normal, input.world_position, input.camera_position);
    let cue = vec3f(input.depth, length(input.camera_position - input.world_position),
        length(input.world_position));
    let color = vec4f(apply_depth_cue(shaded.rgb, cue), clamp(shaded.a, 0.0, 1.0));
    var output: FragmentOut;
    output.accum = scene_wboit_accum(color);
    output.weight = scene_wboit_weight(color.a);
    return output;
}
