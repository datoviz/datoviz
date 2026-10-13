fn point_disc_distance(corner: vec2f, size: f32) -> f32 {
    let point_size = max(size, 0.0);
    let sprite_size = max(point_size + 4.0, 1.0);
    return length(corner * 0.5 * sprite_size) - 0.5 * point_size;
}

fn point_disc_coverage(dist: f32) -> f32 {
    let aa = max(fwidth(dist), 1e-6);
    return 1.0 - smoothstep(-aa, aa, dist);
}
