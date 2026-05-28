// datoviz-builtin-shader: scene.primitive lit fragment v1

#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    return evaluate_scene_material(
        input.color, input.normal, input.world_position, input.camera_position);
}
