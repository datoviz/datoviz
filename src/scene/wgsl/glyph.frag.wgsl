struct FragmentIn {
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
}

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;

fn median3(r: f32, g: f32, b: f32) -> f32 {
    return max(min(r, g), min(max(r, g), b));
}

fn screenPixelRange(uv: vec2f) -> f32 {
    let pixelRange = 4.0;
    let dims = vec2f(textureDimensions(tex, 0));
    let unitRange = vec2f(pixelRange) / dims;
    let screenTexSize = vec2f(1.0) / fwidth(uv);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let texel = textureSample(tex, samp, input.uv);
    var opacity = texel.a;
    if (max(max(texel.r, texel.g), texel.b) > 1.0 / 255.0) {
        var sd = median3(texel.r, texel.g, texel.b);
        let trueSd = texel.a;
        if ((sd - 0.5) * (trueSd - 0.5) < 0.0) {
            sd = trueSd;
        }
        let screenPxDistance = screenPixelRange(input.uv) * (sd - 0.5);
        opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    }
    return vec4f(input.color.rgb, input.color.a * opacity);
}
