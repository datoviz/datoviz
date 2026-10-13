#include "mesh_surface.wgsl"
#include "wboit.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) depth: f32,
    @location(4) world_position: vec3f,
    @location(5) camera_position: vec3f,
}

struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let color = evaluate_mesh_surface(
        input.color, input.normal, input.uv, input.depth,
        input.world_position, input.camera_position);
    if (!scene_alpha_valid(color.a, material.model)) {
        discard;
    }
    var output: FragmentOut;
    output.accum = scene_wboit_accum(color);
    output.weight = scene_wboit_weight(color.a);
    return output;
}
