struct FragmentIn {
    @location(0) id: u32,
}

@fragment
fn main(input: FragmentIn) -> @location(0) u32 {
    return input.id;
}
