@group(0) @binding(0) var accum_texture: texture_2d<f32>;
@group(0) @binding(1) var weight_texture: texture_2d<f32>;
@group(0) @binding(2) var texture_sampler: sampler;

struct FragmentIn {
    @location(0) local_uv: vec2f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let extent = textureDimensions(accum_texture);
    let texel = clamp(vec2i(input.local_uv * vec2f(extent)), vec2i(0), vec2i(extent) - vec2i(1));
    let accum = textureLoad(accum_texture, texel, 0);
    let weight = textureLoad(weight_texture, texel, 0).r;
    let alpha = clamp(1.0 - exp(-weight), 0.0, 1.0);
    var rgb = vec3f(0.0);
    if (accum.a > 1e-5) {
        rgb = accum.rgb / max(accum.a, 1e-5);
    }
    return vec4f(rgb, alpha);
}
