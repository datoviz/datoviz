#include "color.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) sprite: vec2f,
    @location(2) size: f32,
    @location(3) angle: f32,
    @location(4) @interpolate(flat) shape: u32,
    @location(5) tex_rect: vec4f,
}

@group(1) @binding(0) var tex: texture_2d<f32>;
@group(1) @binding(1) var samp: sampler;

struct GlyphParams {
    params: vec4f,
}

@group(1) @binding(2) var<uniform> glyph: GlyphParams;

fn median3(r: f32, g: f32, b: f32) -> f32 {
    return max(min(r, g), min(max(r, g), b));
}

fn screen_pixel_range(uv: vec2f) -> f32 {
    let pixel_range = max(glyph.params.x, 1.0);
    let unit_range = vec2f(pixel_range) / vec2f(textureDimensions(tex));
    let screen_tex_size = vec2f(1.0) / fwidth(uv);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let c = cos(input.angle);
    let s = sin(input.angle);
    let p = vec2f(
        c * input.sprite.x - s * input.sprite.y,
        s * input.sprite.x + c * input.sprite.y,
    );
    if max(abs(p.x), abs(p.y)) > 1.0 {
        discard;
    }
    let marker_uv = p * 0.5 + 0.5;
    let uv = mix(input.tex_rect.xy, input.tex_rect.zw, marker_uv);
    let texel = textureSample(tex, samp, uv);
    var sd = texel.r;
    if i32(glyph.params.y + 0.5) == 2 {
        sd = median3(texel.r, texel.g, texel.b);
    }
    let opacity = clamp(screen_pixel_range(uv) * (sd - 0.5) + 0.5, 0.0, 1.0);
    let color = semantic_color_to_linear(input.color);
    let output = vec4f(color.rgb, color.a * opacity);
    if output.a <= 0.0 {
        discard;
    }
    return output;
}
