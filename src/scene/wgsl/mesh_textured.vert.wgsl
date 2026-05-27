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
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
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
    return output;
}
