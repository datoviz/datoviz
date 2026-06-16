struct SceneMaterial {
    light_dir: vec4f,
    params: vec4f,
    model: vec4f,
    base_color_factor: vec4f,
    standard_params: vec4f,
    emissive_rim: vec4f,
    depth_cue: vec4f,
    depth_cue_color: vec4f,
    depth_cue_extra: vec4f,
}
struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) size: f32,
}

@group(1) @binding(0) var<uniform> material: SceneMaterial;

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = length(input.corner);
    let aa = max(fwidth(dist), 1e-6);
    let outer = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, dist);
    if (outer <= 0.0) {
        discard;
    }

    let line_width = max(material.params.x, 0.0);
    let aspect = u32(material.params.y + 0.5);
    let filled = aspect == 0u || aspect == 2u;
    let stroke = aspect == 1u || aspect == 2u;
    let stroke_width = select(0.0, max(line_width, 1.0), stroke);
    let inner_radius = max(1.0 - 2.0 * stroke_width / max(input.size, 1.0), 0.0);
    let edge_mix = select(0.0, smoothstep(inner_radius - aa, inner_radius + aa, dist), stroke);
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
