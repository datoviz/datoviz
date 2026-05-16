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
    let n = normalize(input.normal);
    let l = normalize(material.light_direction.xyz);
    let v = normalize(input.camera_position - input.world_position);
    let h = normalize(l + v);
    let lambert = max(dot(n, l), 0.0);
    let specular = pow(max(dot(n, h), 0.0), 32.0);
    let rgb = input.color.rgb * (material.params.x + material.params.y * lambert) +
        vec3f(0.18 * specular);
    let cue = vec3f(
        input.depth,
        length(input.camera_position - input.world_position),
        length(input.world_position)
    );
    return vec4f(apply_depth_cue(clamp(rgb, vec3f(0.0), vec3f(1.0)), cue), input.color.a);
}
