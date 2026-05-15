struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    if (dot(input.corner, input.corner) > 1.0) {
        discard;
    }
    return input.color;
}
