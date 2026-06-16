#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) coord: vec2f,
    @location(2) length_px: f32,
    @location(3) line_width: f32,
    @location(4) has_prev: f32,
    @location(5) has_next: f32,
    @location(6) bevel_distance: vec2f,
    @location(7) join_split_distance: f32,
}

fn stroke_alpha(distance: f32, line_width: f32) -> f32 {
    let aa = 1.0;
    let half_width = max(line_width, 0.0) * 0.5;
    return 1.0 - smoothstep(half_width - aa, half_width + aa, abs(distance));
}

fn stroke_outer_half_width(line_width: f32) -> f32 {
    return max(line_width, 0.0) * 0.5 + 1.5;
}

fn stroke_triangle_head_length(line_width: f32) -> f32 {
    return max(max(line_width, 0.0) * 3.0, stroke_outer_half_width(line_width));
}

fn stroke_triangle_head_half_width(line_width: f32) -> f32 {
    return max(max(line_width, 0.0) * 1.5, stroke_outer_half_width(line_width));
}

fn stroke_cap_distance(cap: i32, dx: f32, dy: f32, line_width: f32) -> f32 {
    let aa = 1.0;
    let half_width = max(line_width, 0.0) * 0.5;
    let t = max(half_width - aa, 0.0);
    let x = abs(dx);
    let y = abs(dy);

    if (cap == 0) {
        return 1e6;
    }
    if (cap == 1) {
        return length(vec2f(x, y));
    }
    if (cap == 2) {
        return max(y, t + x - y);
    }
    if (cap == 4) {
        return max(x, y);
    }
    return max(x + t, y);
}

fn stroke_triangle_out_alpha(dx: f32, dy: f32, line_width: f32) -> f32 {
    let aa = 1.0;
    let x = max(dx, 0.0);
    let y = abs(dy);
    let length_px = stroke_triangle_head_length(line_width);
    let half_width = stroke_triangle_head_half_width(line_width);
    let scale = min(length_px, half_width);
    let edge_distance = (x / max(length_px, 1e-6) + y / max(half_width, 1e-6) - 1.0) * scale;
    let tip_distance = x - length_px;
    let distance = max(edge_distance, tip_distance);
    return 1.0 - smoothstep(-aa, aa, distance);
}

fn stroke_cap_alpha(cap: i32, dx: f32, dy: f32, line_width: f32) -> f32 {
    if (cap == 3) {
        return stroke_triangle_out_alpha(dx, dy, line_width);
    }
    return stroke_alpha(stroke_cap_distance(cap, dx, dy, line_width), line_width);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    var distance = input.coord.y;
    var alpha = stroke_alpha(distance, input.line_width);
    let join_type = i32(material.params.z + 0.5);
    let aa = 1.0;
    if (input.coord.x < 0.0) {
        if (input.has_prev >= 0.5 && join_type == 1) {
            distance = length(input.coord);
            alpha = stroke_alpha(distance, input.line_width);
        } else if (input.has_prev < 0.5) {
            let cap_type = i32(material.params.x + 0.5);
            alpha = stroke_cap_alpha(cap_type, -input.coord.x, input.coord.y, input.line_width);
            distance = stroke_cap_distance(cap_type, input.coord.x, input.coord.y, input.line_width);
        }
    } else if (input.coord.x > input.length_px) {
        if (input.has_next >= 0.5 && join_type == 1) {
            distance = length(input.coord - vec2f(input.length_px, 0.0));
            alpha = stroke_alpha(distance, input.line_width);
        } else if (input.has_next < 0.5) {
            let cap_type = i32(material.params.y + 0.5);
            alpha = stroke_cap_alpha(
                cap_type, input.coord.x - input.length_px, input.coord.y, input.line_width);
            distance = stroke_cap_distance(
                cap_type, input.coord.x - input.length_px, input.coord.y, input.line_width);
        }
    }

    let miter_limit = max(material.params.w, 1.0);
    let miter_clip = (miter_limit - 1.0) * (input.line_width * 0.5) + aa;
    let bevel_clip = aa;
    let start_bevel = join_type == 2 && input.coord.x < 0.0 && input.has_prev >= 0.5;
    let end_bevel = join_type == 2 && input.coord.x > input.length_px && input.has_next >= 0.5;
    if (start_bevel) {
        if (input.bevel_distance.x > abs(distance) + bevel_clip) {
            distance = input.bevel_distance.x - bevel_clip;
        }
        alpha = stroke_alpha(distance, input.line_width);
    } else if (end_bevel) {
        if (input.bevel_distance.y > abs(distance) + bevel_clip) {
            distance = input.bevel_distance.y - bevel_clip;
        }
        alpha = stroke_alpha(distance, input.line_width);
    } else if (input.coord.x < 0.0 && input.has_prev >= 0.5 && join_type == 0) {
        if (input.bevel_distance.x > abs(distance) + miter_clip) {
            distance = input.bevel_distance.x - miter_clip;
        }
        alpha = stroke_alpha(distance, input.line_width);
    } else if (input.coord.x > input.length_px && input.has_next >= 0.5 && join_type == 0) {
        if (input.bevel_distance.y > abs(distance) + miter_clip) {
            distance = input.bevel_distance.y - miter_clip;
        }
        alpha = stroke_alpha(distance, input.line_width);
    }
    if (alpha <= 0.0) {
        discard;
    }
    let linear_color = semantic_color_to_linear(input.color);
    let color = vec4f(linear_color.rgb, linear_color.a * alpha);
    if (color.a <= 0.0) {
        discard;
    }
    return color;
}
