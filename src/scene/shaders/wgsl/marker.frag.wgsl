#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) sprite: vec2f,
    @location(2) size: f32,
    @location(3) angle: f32,
    @location(4) @interpolate(flat) shape: u32,
}

fn sd_triangle(p_in: vec2f) -> f32 {
    let k = 1.7320508;
    var p = vec2f(abs(p_in.x) - 1.0, p_in.y + 1.0 / k);
    if (p.x + k * p.y > 0.0) {
        p = vec2f(p.x - k * p.y, -k * p.x - p.y) * 0.5;
    }
    p.x -= clamp(p.x, -2.0, 0.0);
    return -length(p) * sign(p.y);
}

fn sd_box(p: vec2f, b: vec2f) -> f32 {
    let d = abs(p) - b;
    return length(max(d, vec2f(0.0))) + min(max(d.x, d.y), 0.0);
}

fn sd_round_box(p: vec2f, b: vec2f, r: f32) -> f32 {
    let q = abs(p) - b + vec2f(r);
    return length(max(q, vec2f(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

fn sd_segment(p: vec2f, a: vec2f, b: vec2f) -> f32 {
    let pa = p - a;
    let ba = b - a;
    let h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

fn sd_ellipse(p: vec2f, r: vec2f) -> f32 {
    return (length(p / r) - 1.0) * min(r.x, r.y);
}

fn sd_heart(p_in: vec2f) -> f32 {
    var p = vec2f(p_in.x, -p_in.y) * 0.82 + vec2f(0.0, 0.18);
    p.x = abs(p.x);
    if (p.y + p.x > 1.0) {
        return length(p - vec2f(0.25, 0.75)) - 0.3535534;
    }
    let h = max(p.x + p.y, 0.0);
    let d0 = dot(p - vec2f(0.0, 1.0), p - vec2f(0.0, 1.0));
    let d1 = dot(p - vec2f(0.5 * h), p - vec2f(0.5 * h));
    return sqrt(min(d0, d1)) * sign(p.x - p.y);
}

fn marker_distance(p: vec2f, shape: u32) -> f32 {
    if (shape == 1u) {
        return sd_box(p, vec2f(1.0));
    }
    if (shape == 2u) {
        return sd_triangle(p);
    }
    if (shape == 3u) {
        return abs(p.x) + abs(p.y) - 1.0;
    }
    if (shape == 4u) {
        return min(sd_box(p, vec2f(0.28, 1.0)), sd_box(p, vec2f(1.0, 0.28)));
    }
    if (shape == 5u) {
        return abs(length(p) - 0.62) - 0.18;
    }
    if (shape == 6u) {
        let ring = abs(length(p) - 0.48) - 0.04;
        let h = sd_box(p, vec2f(1.0, 0.028));
        let v = sd_box(p, vec2f(0.028, 1.0));
        let inner = length(p) - 0.24;
        let crosshair = max(min(h, v), -inner);
        return min(ring, crosshair);
    }
    if (shape == 7u) {
        let a = sd_segment(p, vec2f(-0.9, 0.0), vec2f(0.9, 0.0));
        let b = sd_segment(p, vec2f(-0.45, -0.78), vec2f(0.45, 0.78));
        let c = sd_segment(p, vec2f(-0.45, 0.78), vec2f(0.45, -0.78));
        return min(a, min(b, c)) - 0.09;
    }
    if (shape == 8u) {
        let a = sd_segment(p, vec2f(-0.72, 0.58), vec2f(0.0, -0.42));
        let b = sd_segment(p, vec2f(0.0, -0.42), vec2f(0.72, 0.58));
        return min(a, b) - 0.13;
    }
    if (shape == 9u) {
        var d = length(p - vec2f(0.0, 0.46)) - 0.43;
        d = min(d, length(p - vec2f(0.46, 0.0)) - 0.43);
        d = min(d, length(p - vec2f(0.0, -0.46)) - 0.43);
        d = min(d, length(p - vec2f(-0.46, 0.0)) - 0.43);
        return d;
    }
    if (shape == 10u) {
        var d = length(p - vec2f(0.0, 0.36)) - 0.36;
        d = min(d, length(p - vec2f(-0.34, -0.08)) - 0.36);
        d = min(d, length(p - vec2f(0.34, -0.08)) - 0.36);
        d = min(d, sd_box(p - vec2f(0.0, -0.58), vec2f(0.16, 0.38)));
        return d;
    }
    if (shape == 11u) {
        let shaft = sd_box(p - vec2f(-0.28, 0.0), vec2f(0.58, 0.18));
        let head = sd_triangle(vec2f(-p.y * 1.35, (p.x - 0.12) * 1.35));
        return min(shaft, head);
    }
    if (shape == 12u) {
        return sd_ellipse(p, vec2f(0.98, 0.56));
    }
    if (shape == 13u) {
        return sd_box(p, vec2f(1.0, 0.28));
    }
    if (shape == 14u) {
        return sd_heart(p);
    }
    if (shape == 15u) {
        let left = abs(length(p - vec2f(-0.42, 0.0)) - 0.35) - 0.12;
        let right = abs(length(p - vec2f(0.42, 0.0)) - 0.35) - 0.12;
        return min(left, right);
    }
    if (shape == 16u) {
        let head = length(p - vec2f(0.0, 0.28)) - 0.5;
        let tip = sd_triangle(vec2f(p.x * 1.15, p.y * 1.15 + 0.3));
        let hole = -(length(p - vec2f(0.0, 0.30)) - 0.18);
        return max(min(head, tip), hole);
    }
    if (shape == 17u) {
        var d = sd_heart(vec2f(p.x, -p.y + 0.08));
        d = min(d, sd_box(p - vec2f(0.0, -0.62), vec2f(0.16, 0.34)));
        return d;
    }
    if (shape == 18u) {
        let tag = max(sd_round_box(p - vec2f(0.08, 0.0), vec2f(0.86, 0.58), 0.12), p.x + p.y - 1.0);
        let hole = -(length(p - vec2f(-0.54, 0.0)) - 0.12);
        return max(tag, hole);
    }
    if (shape == 19u) {
        return sd_box(p, vec2f(0.28, 1.0));
    }
    if (shape == 20u) {
        return sd_round_box(p, vec2f(0.96, 0.68), 0.18);
    }
    return length(p) - 1.0;
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let c = cos(input.angle);
    let s = sin(input.angle);
    let p = vec2f(c * input.sprite.x - s * input.sprite.y, s * input.sprite.x + c * input.sprite.y);

    let dist = marker_distance(p, input.shape);
    let aa = max(fwidth(dist), 1e-6);
    let outer = 1.0 - smoothstep(-aa, aa, dist);
    if (outer <= 0.0) {
        discard;
    }

    let line_width = max(material.params.x, 0.0);
    let aspect = i32(material.params.y + 0.5);
    let filled = aspect == 0 || aspect == 2;
    let stroke = aspect == 1 || aspect == 2;
    let stroke_width_px = select(0.0, max(2.0 * max(line_width, 1.0) / max(input.size, 1.0), aa), stroke);
    let edge_mask = select(0.0, 1.0 - smoothstep(stroke_width_px - aa, stroke_width_px + aa, -dist), stroke);
    let fill_mask = select(0.0, 1.0 - edge_mask, filled);
    let stroke_mask = select(0.0, edge_mask, stroke);
    let coverage = outer * max(fill_mask, stroke_mask);
    if (coverage <= 0.0) {
        discard;
    }

    let edge_color = semantic_color_to_linear(material.base_color_factor);
    let input_color = semantic_color_to_linear(input.color);
    let color = select(edge_color, mix(input_color, edge_color, stroke_mask), filled);
    let out_color = vec4f(color.rgb, color.a * coverage);
    if (out_color.a <= 0.0) {
        discard;
    }
    return out_color;
}
