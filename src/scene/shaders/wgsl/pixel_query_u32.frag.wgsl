struct FragmentIn {
    @location(0) @interpolate(flat) id: u32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) u32 {
    return input.id;
}
