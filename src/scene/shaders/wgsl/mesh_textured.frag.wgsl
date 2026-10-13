// datoviz-builtin-shader: scene.mesh textured fragment v1

#include "mesh_surface.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) depth: f32,
    @location(4) world_position: vec3f,
    @location(5) camera_position: vec3f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let color = evaluate_mesh_surface(
        input.color, input.normal, input.uv, input.depth,
        input.world_position, input.camera_position);
    if (!scene_alpha_valid(color.a, material.model)) {
        discard;
    }
    return color;
}
