// datoviz-builtin-shader: scene.mesh textured fragment v1

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) depth: f32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let texel = textureSample(tex, samp, input.uv);
    let base = texel * input.color;
    let n = normalize(input.normal);
    let l = normalize(vec3f(0.35, 0.55, 0.75));
    let diffuse = max(dot(n, l), 0.0);
    let rgb = base.rgb * (0.28 + 0.72 * diffuse);
    return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), base.a);
}
