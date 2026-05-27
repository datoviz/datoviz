struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) offset_px: vec2f,
    @location(2) sigma: vec2f,
    @location(3) angle: f32,
}

struct FragmentOut {
    @location(0) accum: vec4f,
    @location(1) weight: f32,
}

const CUTOFF_SIGMA: f32 = 3.0;

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let c = cos(input.angle);
    let s = sin(input.angle);
    let local = vec2f(
        c * input.offset_px.x + s * input.offset_px.y,
        -s * input.offset_px.x + c * input.offset_px.y
    );
    let q = local / max(input.sigma, vec2f(0.000001));
    let q2 = dot(q, q);
    if (q2 > CUTOFF_SIGMA * CUTOFF_SIGMA) {
        discard;
    }

    let alpha = input.color.a * exp(-0.5 * q2);
    if (alpha <= 0.0) {
        discard;
    }

    let a = clamp(alpha, 0.0, 1.0);
    var output: FragmentOut;
    output.accum = vec4f(input.color.rgb * a, a);
    output.weight = -log(max(1.0 - a, 1e-4));
    return output;
}
