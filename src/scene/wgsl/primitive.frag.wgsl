struct FragmentIn {
    @location(0) color: vec4f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    return input.color;
}
