fn scene_wboit_accum(linear_color: vec4f) -> vec4f {
    let alpha = clamp(linear_color.a, 0.0, 1.0);
    return vec4f(linear_color.rgb * alpha, alpha);
}

fn scene_wboit_weight(alpha: f32) -> f32 {
    return -log(max(1.0 - clamp(alpha, 0.0, 1.0), 1e-4));
}
