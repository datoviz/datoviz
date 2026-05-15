struct FragmentIn {
    @location(0) uv: vec2f,
}

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    return textureSample(tex, samp, input.uv);
}
