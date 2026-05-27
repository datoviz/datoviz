struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) offset_px: vec2f,
    @location(2) sigma: vec2f,
}

const CUTOFF_SIGMA: f32 = 3.0;

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let q = input.offset_px / max(input.sigma, vec2f(0.000001));
    let q2 = dot(q, q);
    if (q2 > CUTOFF_SIGMA * CUTOFF_SIGMA) {
        discard;
    }

    let alpha = input.color.a * exp(-0.5 * q2);
    if (alpha <= 0.0) {
        discard;
    }
    return vec4f(input.color.rgb, alpha);
}
