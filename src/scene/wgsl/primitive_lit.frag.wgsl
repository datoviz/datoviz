// datoviz-builtin-shader: scene.primitive lit fragment v1

struct SceneMaterial {
    light_direction: vec4f,
    params: vec4f,
}

@group(1) @binding(0) var<uniform> material: SceneMaterial;

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
    return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), input.color.a);
}
