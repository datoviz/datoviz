fn srgb_to_linear(srgb: vec3f) -> vec3f {
    let clipped = clamp(srgb, vec3f(0.0), vec3f(1.0));
    let lo = clipped / vec3f(12.92);
    let hi = pow((clipped + vec3f(0.055)) / vec3f(1.055), vec3f(2.4));
    return select(hi, lo, clipped <= vec3f(0.04045));
}

fn semantic_color_to_linear(color: vec4f) -> vec4f {
    let clipped = clamp(color.rgb, vec3f(0.0), vec3f(1.0));
    if (DVZ_LEGACY_SRGB_BLEND) {
        return vec4f(clipped, clamp(color.a, 0.0, 1.0));
    }
    return vec4f(srgb_to_linear(clipped), clamp(color.a, 0.0, 1.0));
}

fn sampled_texture_color_to_linear(color: vec4f, params: vec4f) -> vec4f {
    if (params.x > 0.5) {
        return semantic_color_to_linear(color);
    }
    return vec4f(color.rgb, clamp(color.a, 0.0, 1.0));
}
