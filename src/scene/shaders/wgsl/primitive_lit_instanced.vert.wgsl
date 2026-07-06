// datoviz-builtin-shader: scene.primitive lit_instanced vertex v1

#include "common.wgsl"
#include "camera.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) normal: vec3f,
    @location(3) instance_transform0: vec4f,
    @location(4) instance_transform1: vec4f,
    @location(5) instance_transform2: vec4f,
    @location(6) instance_transform3: vec4f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
}

fn instance_transform(input: VertexIn) -> mat4x4f {
    return mat4x4f(
        input.instance_transform0,
        input.instance_transform1,
        input.instance_transform2,
        input.instance_transform3,
    );
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    let model = mvp.model * instance_transform(input);
    let world = model * vec4f(input.position, 1.0);
    let clip = mvp.proj * mvp.view * world;

    var output: VertexOut;
    output.position = clip;
    output.color = input.color;
    output.normal = transpose(dvz_inverse_mat3x3f(mat3x3f(
        model[0].xyz,
        model[1].xyz,
        model[2].xyz,
    ))) * input.normal;
    output.world_position = world.xyz;
    output.camera_position = camera_position_from_view();
    output.depth = clip.z / max(abs(clip.w), 1e-6);
    return output;
}
