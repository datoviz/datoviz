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
    let texel = semantic_color_to_linear(textureSample(tex, samp, uv));
    let color = semantic_color_to_linear(input.color);
    let output = vec4f(color.rgb * texel.rgb, color.a * texel.a);
    if output.a <= 0.0 {
        discard;
    }
    return output;
}
