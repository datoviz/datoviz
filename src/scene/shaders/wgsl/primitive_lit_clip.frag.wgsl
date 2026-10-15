#include "mesh_fragment_clip.wgsl"
// datoviz-builtin-shader: scene.primitive lit fragment v1

#include "scene_material.wgsl"

struct FragmentIn {
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    if (!mesh_fragment_covered(input.clip_distance, input.clip_include_zero)) { discard; }
    return evaluate_scene_material(
        input.color, input.normal, input.world_position, input.camera_position);
}
