// datoviz-builtin-shader: scene.primitive lit_indexed_item_state vertex v1

#include "common.wgsl"
#include "camera.wgsl"

#include "item_state_style.wgsl"

@group(1) @binding(2) var<storage, read> item_state_words: array<u32>;

struct VertexIn {
    @location(8) fragment_clip: vec2f,
    @location(7) state_index: u32,
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) normal: vec3f,
}

struct VertexOut {
    @location(5) clip_distance: f32,
    @location(6) @interpolate(flat) clip_include_zero: u32,
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.clip_distance = input.fragment_clip.x;
    output.clip_include_zero = u32(input.fragment_clip.y);
    let world = mvp.model * vec4f(input.position, 1.0);
    let clip = transform(input.position);
    output.position = clip;
    output.color = apply_item_state_color(input.color, item_state_words[input.state_index]);
    output.world_position = world.xyz;
    output.camera_position = camera_position_from_view();
    output.normal = mat3x3f(
        mvp.model[0].xyz,
        mvp.model[1].xyz,
        mvp.model[2].xyz
    ) * input.normal;
    output.depth = clip.z / max(abs(clip.w), 1e-6);
    return output;
}
