struct FragmentIn {
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
}

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;

fn median3(r: f32, g: f32, b: f32) -> f32 {
    return max(min(r, g), min(max(r, g), b));
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let texel = textureSample(tex, samp, input.uv);
    let sd = median3(texel.r, texel.g, texel.b);
    let w = max(fwidth(sd), 1.0 / 255.0);
    var opacity = smoothstep(0.5 - w, 0.5 + w, sd);
    if (texel.a > 0.0 && abs(texel.a - sd) > 1.0 / 255.0) {
        opacity = max(opacity, smoothstep(0.5 - w, 0.5 + w, texel.a));
    }
    return vec4f(input.color.rgb, input.color.a * opacity);
}
