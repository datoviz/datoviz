// datoviz-builtin-shader: scene.mesh textured vertex v1

#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) normal: vec3f,
    @location(3) uv: vec2f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) depth: f32,
    @location(4) world_position: vec3f,
    @location(5) camera_position: vec3f,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    let world = mvp.model * vec4f(input.position, 1.0);
    let clip = transform(input.position);
    output.position = clip;
    output.color = input.color;
    output.normal = mat3x3f(
        mvp.model[0].xyz,
        mvp.model[1].xyz,
        mvp.model[2].xyz
    ) * input.normal;
    output.uv = input.uv;
    output.depth = clip.z / max(abs(clip.w), 1e-6);
    output.world_position = world.xyz;
    output.camera_position = vec3f(0.0, 0.0, 3.0);
    return output;
}
