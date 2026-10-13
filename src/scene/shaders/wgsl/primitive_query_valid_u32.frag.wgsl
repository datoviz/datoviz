// datoviz-builtin-shader: scene.primitive query_valid_u32 fragment v1

struct FragmentIn {
    @location(1) alpha: f32,
    @location(0) @interpolate(flat) id: u32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) u32 {
    if (!(input.alpha > 0.0) || input.alpha > 3.402823466e38) { discard; }
    return input.id;
}
