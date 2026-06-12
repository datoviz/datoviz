#include "common.wgsl"
#include "scene_material.wgsl"

struct VertexIn {
    @location(0) position_prev: vec3f,
    @location(1) position_start: vec3f,
    @location(2) position_end: vec3f,
    @location(3) position_next: vec3f,
    @location(4) color: vec4f,
    @location(5) line_width: f32,
    @location(6) path_flags: u32,
    @location(7) path_distance: f32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) coord: vec2f,
    @location(2) length_px: f32,
    @location(3) line_width: f32,
    @location(4) has_prev: f32,
    @location(5) has_next: f32,
    @location(6) bevel_distance: vec2f,
    @location(7) join_split_distance: f32,
}

const SIDE_NEGATIVE: u32 = 0x01u;
const ENDPOINT_END: u32 = 0x02u;
const HAS_PREV: u32 = 0x04u;
const HAS_NEXT: u32 = 0x08u;

fn clip_to_pixel(clip: vec4f) -> vec2f {
    let ndc = clip.xy / max(abs(clip.w), 1e-6);
    return (ndc * 0.5 + vec2f(0.5)) * viewport.rect.zw;
}

fn pixel_to_clip(pixel: vec2f, depth: f32) -> vec4f {
    let ndc = pixel / max(viewport.rect.zw, vec2f(1.0)) * 2.0 - vec2f(1.0);
    return vec4f(ndc, depth, 1.0);
}

fn safe_normalize(v: vec2f, fallback: vec2f) -> vec2f {
    let n = length(v);
    if (n <= 1e-6) {
        return fallback;
    }
    return v / n;
}

fn line_distance(p0: vec2f, p1: vec2f, p: vec2f) -> f32 {
    let v = p1 - p0;
    let l2 = max(dot(v, v), 1e-6);
    let u = dot(p - p0, v) / l2;
    let h = p0 + u * v;
    return length(p - h);
}

fn compute_u(p0: vec2f, p1: vec2f, p: vec2f) -> f32 {
    let v = p1 - p0;
    let l = max(length(v), 1e-6);
    return dot(p - p0, v) / l;
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

fn stroke_cap_extension(cap: i32, line_width: f32) -> f32 {
    if (cap == 0 || cap == 5) {
        return 0.0;
    }
    if (cap == 3) {
        return stroke_triangle_head_length(line_width) + 1.0;
    }
    return stroke_outer_half_width(line_width);
}

fn stroke_cap_half_width(cap: i32, line_width: f32) -> f32 {
    if (cap == 3) {
        return stroke_triangle_head_half_width(line_width) + 1.0;
    }
    return stroke_outer_half_width(line_width);
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    let side_negative = (input.path_flags & SIDE_NEGATIVE) != 0u;
    let endpoint_end = (input.path_flags & ENDPOINT_END) != 0u;
    let has_prev = (input.path_flags & HAS_PREV) != 0u;
    let has_next = (input.path_flags & HAS_NEXT) != 0u;
    let side = select(1.0, -1.0, side_negative);

    let p0_clip = transform(input.position_prev);
    let p1_clip = transform(input.position_start);
    let p2_clip = transform(input.position_end);
    let p3_clip = transform(input.position_next);
    let p0 = clip_to_pixel(p0_clip);
    let p1 = clip_to_pixel(p1_clip);
    let p2 = clip_to_pixel(p2_clip);
    let p3 = clip_to_pixel(p3_clip);

    var v0 = safe_normalize(p1 - p0, vec2f(1.0, 0.0));
    let v1 = safe_normalize(p2 - p1, v0);
    var v2 = safe_normalize(p3 - p2, v1);
    if (!has_prev) {
        v0 = v1;
    }
    if (!has_next) {
        v2 = v1;
    }
    let n0 = vec2f(-v0.y, v0.x);
    let n1 = vec2f(-v1.y, v1.x);
    let n2 = vec2f(-v2.y, v2.x);

    let stroke_width = max(input.line_width, 0.0);
    let half_width = stroke_outer_half_width(stroke_width);
    let length_px = length(p2 - p1);
    let miter_start = safe_normalize(n0 + n1, n1);
    let miter_end = safe_normalize(n1 + n2, n1);
    let denom_start = dot(miter_start, n1);
    let denom_end = dot(miter_end, n1);
    let length_start = select(half_width, half_width / denom_start, denom_start > 1e-3);
    let length_end = select(half_width, half_width / denom_end, denom_end > 1e-3);

    let cap_type = select(i32(material.params.x + 0.5), i32(material.params.y + 0.5), endpoint_end);
    var output: VertexOut;
    var pixel = p1;
    if (!endpoint_end) {
        if (!has_prev) {
            let cap_extension = stroke_cap_extension(cap_type, stroke_width);
            let cap_half_width = stroke_cap_half_width(cap_type, stroke_width);
            pixel = p1 - cap_extension * v1 + side * cap_half_width * n1;
            output.coord = vec2f(-cap_extension, side * cap_half_width);
        } else {
            pixel = p1 + side * length_start * miter_start;
            output.coord = vec2f(compute_u(p1, p2, pixel), side * half_width);
        }
    } else {
        if (!has_next) {
            let cap_extension = stroke_cap_extension(cap_type, stroke_width);
            let cap_half_width = stroke_cap_half_width(cap_type, stroke_width);
            pixel = p2 + cap_extension * v1 + side * cap_half_width * n1;
            output.coord = vec2f(length_px + cap_extension, side * cap_half_width);
        } else {
            pixel = p2 + side * length_end * miter_end;
            output.coord = vec2f(compute_u(p1, p2, pixel), side * half_width);
        }
    }
    let depth = select(
        p1_clip.z / max(abs(p1_clip.w), 1e-6),
        p2_clip.z / max(abs(p2_clip.w), 1e-6),
        endpoint_end);
    output.position = pixel_to_clip(pixel, depth);
    output.color = input.color;
    output.bevel_distance = vec2f(-half_width, -half_width);
    output.join_split_distance = 0.0;
    let turn_start = v0.x * v1.y - v0.y * v1.x;
    let turn_end = v1.x * v2.y - v1.y * v2.x;
    let d0 = select(1.0, -1.0, turn_start > 0.0);
    let d1 = select(1.0, -1.0, turn_end > 0.0);
    let start_distance = line_distance(p1 + d0 * n0 * half_width, p1 + d0 * n1 * half_width, pixel);
    let end_distance = line_distance(p2 + d1 * n1 * half_width, p2 + d1 * n2 * half_width, pixel);
    output.bevel_distance.x = select(-start_distance, select(side * d0 * start_distance, -start_distance, endpoint_end), has_prev);
    output.bevel_distance.y = select(-end_distance, select(-end_distance, -side * d1 * end_distance, endpoint_end), has_next);
    output.length_px = length_px;
    output.line_width = stroke_width;
    output.has_prev = select(0.0, 1.0, has_prev);
    output.has_next = select(0.0, 1.0, has_next);
    return output;
}
