struct FragmentIn {
    @location(0) id: u32,
    @location(1) corner: vec2f,
    @location(2) size: f32,
}

fn point_disc_distance(corner: vec2f, size: f32) -> f32 {
    let point_size = max(size, 0.0);
    let sprite_size = max(point_size + 4.0, 1.0);
    return length(corner * 0.5 * sprite_size) - 0.5 * point_size;
}

@fragment
fn main(input: FragmentIn) -> @location(0) u32 {
    if (point_disc_distance(input.corner, input.size) > 0.0) {
        discard;
    }
    return input.id;
}
