#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) size: f32,
}

fn point_disc_distance(corner: vec2f, size: f32) -> f32 {
    let point_size = max(size, 0.0);
    let sprite_size = max(point_size + 4.0, 1.0);
    return length(corner * 0.5 * sprite_size) - 0.5 * point_size;
}

fn point_disc_coverage(dist: f32) -> f32 {
    let aa = max(fwidth(dist), 1e-6);
    return 1.0 - smoothstep(-aa, aa, dist);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = point_disc_distance(input.corner, input.size);
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
    let input_color = semantic_color_to_linear(input.color);
    let color = mix(input_color, edge_color, stroke_mask);
    return vec4f(color.rgb, color.a * coverage);
}
