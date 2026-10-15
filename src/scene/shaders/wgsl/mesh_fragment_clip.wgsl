// Perspective-interpolated authored distance and a triangle-constant boundary policy.
fn mesh_fragment_covered(distance: f32, include_zero: u32) -> bool {
    return distance > 0.0 || (distance == 0.0 && include_zero != 0u);
}
