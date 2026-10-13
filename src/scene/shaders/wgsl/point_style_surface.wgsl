fn point_style_surface(input_color: vec4f, dist: f32) -> vec4f {
    let aa = max(fwidth(dist), 1e-6);
    let outer = point_disc_coverage(dist);
    if (outer <= 0.0) {
        discard;
    }

    let line_width = max(material.params.x, 0.0);
    let aspect = u32(material.params.y + 0.5);
    let filled = aspect == 0u || aspect == 2u;
    let stroke = (aspect == 1u || aspect == 2u) && line_width > 0.0;
    let stroke_width_px = select(0.0, line_width, stroke);
    let edge_mix = select(0.0, smoothstep(-aa, aa, dist + stroke_width_px), stroke);
    let fill_mask = select(0.0, 1.0 - edge_mix, filled);
    let stroke_mask = select(0.0, edge_mix, stroke);
    let coverage = outer * max(fill_mask, stroke_mask);
    if (coverage <= 0.0) {
        discard;
    }

    let edge_color = semantic_color_to_linear(material.base_color_factor);
    let linear_color = semantic_color_to_linear(input_color);
    let color = select(edge_color, mix(linear_color, edge_color, stroke_mask), filled);
    let surface = vec4f(color.rgb, color.a * coverage);
    if (surface.a <= 0.0) {
        discard;
    }
    return surface;
}
