struct FragmentIn {
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
}

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;
@group(1) @binding(2) var<uniform> glyph: vec4f;

const GLYPH_ENCODING_SDF_ALPHA: i32 = 1;
const GLYPH_ENCODING_MSDF_RGB: i32 = 2;

fn median3(r: f32, g: f32, b: f32) -> f32 {
    return max(min(r, g), min(max(r, g), b));
}

fn screenPixelRange(uv: vec2f) -> f32 {
    let pixelRange = max(glyph.x, 1.0);
    let dims = vec2f(textureDimensions(tex, 0));
    let unitRange = vec2f(pixelRange) / dims;
    let screenTexSize = vec2f(1.0) / fwidth(uv);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

fn distanceOpacity(uv: vec2f, sd: f32) -> f32 {
    let screenPxDistance = screenPixelRange(uv) * (sd - 0.5);
    return clamp(screenPxDistance + 0.5, 0.0, 1.0);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let texel = textureSample(tex, samp, input.uv);
    var opacity = texel.a;
    let encoding = i32(glyph.y + 0.5);
    if (encoding == GLYPH_ENCODING_MSDF_RGB) {
        // The generated atlas is MTSDF: RGB carries sharp MSDF, alpha guards sign artifacts.
        let msdf = median3(texel.r, texel.g, texel.b);
        let sdf = texel.a;
        var sd = msdf;
        if ((msdf - 0.5) * (sdf - 0.5) < 0.0) {
            sd = sdf;
        }
        opacity = distanceOpacity(input.uv, sd);
    } else if (encoding == GLYPH_ENCODING_SDF_ALPHA) {
        opacity = distanceOpacity(input.uv, texel.a);
    }
    return vec4f(input.color.rgb, input.color.a * opacity);
}
