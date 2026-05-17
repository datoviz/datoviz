#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) cue: vec3f,
    @location(3) size: f32,
}
@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = length(input.corner);
    let aa = max(fwidth(dist), 1e-6);
    let outer = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, dist);
    if (outer <= 0.0) {
        discard;
    }

    let line_width = max(material.params.x, 0.0);
    let filled = material.params.y > 0.5;
    let stroke = material.params.z > 0.5 || material.params.w > 0.5 || line_width > 0.0;
    let outline = material.params.w > 0.5;
    let stroke_width = select(0.0, max(line_width, 1.0), stroke);
    let inner_radius = max(1.0 - 2.0 * stroke_width / max(input.size, 1.0), 0.0);
    let edge_mix = select(0.0, smoothstep(inner_radius - aa, inner_radius + aa, dist), stroke);
    let fill_mask = select(0.0, 1.0 - edge_mix, filled && !outline);
    let stroke_mask = select(0.0, edge_mix, stroke);
    let coverage = outer * max(fill_mask, stroke_mask);
    if (coverage <= 0.0) {
        discard;
    }

    let edge_color = material.base_color_factor;
    let color = mix(input.color, edge_color, stroke_mask);
    return vec4f(apply_depth_cue(color.rgb, input.cue), color.a * coverage);
}
