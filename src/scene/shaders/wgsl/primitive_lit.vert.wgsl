// datoviz-builtin-shader: scene.primitive lit vertex v1

#include "common.wgsl"
#include "camera.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) normal: vec3f,
}

struct VertexOut {
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
    let world = mvp.model * vec4f(input.position, 1.0);
    let clip = transform(input.position);
    output.position = clip;
    output.color = input.color;
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
