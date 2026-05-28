// datoviz-builtin-shader: scene.mesh textured fragment v1

#include "scene_material.wgsl"

@group(1) @binding(1) var tex: texture_2d<f32>;
@group(1) @binding(2) var samp: sampler;

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
    let texel = textureSample(tex, samp, input.uv);
    let base = texel * input.color;
    let shaded = evaluate_scene_material(
        base, input.normal, input.world_position, input.camera_position);
    let cue = vec3f(
        input.depth, length(input.camera_position - input.world_position),
        length(input.world_position));
    return vec4f(apply_depth_cue(shaded.rgb, cue), shaded.a);
}
