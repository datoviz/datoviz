#include "color.wgsl"

struct FragmentIn {
    @location(0) uv: vec2f,
}

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;
@group(1) @binding(2) var<uniform> texture_params: vec4f;

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let texel = textureSample(tex, samp, input.uv);
    return sampled_texture_color_to_linear(texel, texture_params);
}
