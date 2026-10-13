#include "scene_material.wgsl"
#include "scene_alpha.wgsl"

@group(1) @binding(1) var tex: texture_2d<f32>;
@group(1) @binding(2) var samp: sampler;
@group(1) @binding(3) var<uniform> texture_params: vec4f;

fn evaluate_mesh_surface(
    color: vec4f, normal: vec3f, uv: vec2f, depth: f32,
    world_position: vec3f, camera_position: vec3f) -> vec4f {
    let texel = sampled_texture_color_to_linear(textureSample(tex, samp, uv), texture_params);
    let base = texel * semantic_color_to_linear(color);
    let shaded = evaluate_scene_material_linear_item(base, normal, world_position, camera_position);
    let cue = vec3f(depth, length(camera_position - world_position), length(world_position));
    return vec4f(apply_depth_cue(shaded.rgb, cue), shaded.a);
}
