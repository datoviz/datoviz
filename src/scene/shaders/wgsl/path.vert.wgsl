#include "common.wgsl"
#include "scene_material.wgsl"

struct VertexIn {
    @location(0) position_prev: vec3f,
    @location(1) position_curr: vec3f,
    @location(2) position_next: vec3f,
    @location(3) color: vec4f,
    @location(4) line_width: f32,
    @location(5) path_flags: u32,
    @location(6) path_distance: f32,
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

    let prev_clip = transform(input.position_prev);
    let curr_clip = transform(input.position_curr);
    let next_clip = transform(input.position_next);
    let prev_px = clip_to_pixel(prev_clip);
    let curr_px = clip_to_pixel(curr_clip);
    let next_px = clip_to_pixel(next_clip);

    var dir_in = safe_normalize(curr_px - prev_px, vec2f(1.0, 0.0));
    var dir_out = safe_normalize(next_px - curr_px, dir_in);
    if (!has_prev) {
        dir_in = dir_out;
    }
    if (!has_next) {
        dir_out = dir_in;
    }
    let normal_in = vec2f(-dir_in.y, dir_in.x);
    let normal_out = vec2f(-dir_out.y, dir_out.x);

    let stroke_width = max(input.line_width, 0.0);
    let half_width = stroke_outer_half_width(stroke_width);
    let join_type = i32(material.params.z + 0.5);
    let miter_limit = max(material.params.w, 1.0);

    let tangent = select(dir_out, dir_in, endpoint_end);
    let segment_normal = select(normal_out, normal_in, endpoint_end);
    let length_px = select(length(next_px - curr_px), length(curr_px - prev_px), endpoint_end);
    let along = select(0.0, length_px, endpoint_end);
    var tangent_offset = 0.0;

    var normal = segment_normal;
    var miter = segment_normal;
    var miter_scale = 1.0;
    if (has_prev && has_next) {
        miter = safe_normalize(normal_in + normal_out, segment_normal);
        let denom = dot(miter, segment_normal);
        miter_scale = select(1.0, 1.0 / denom, denom > 1e-3);
        if (join_type == 1) {
            normal = miter * miter_scale;
        } else if (join_type == 0 && miter_scale <= miter_limit) {
            normal = miter * miter_scale;
        }
    }

    let cap_type = select(i32(material.params.x + 0.5), i32(material.params.y + 0.5), endpoint_end);
    var cap_half_width = half_width;
    if (!has_prev && !endpoint_end) {
        tangent_offset = -stroke_cap_extension(cap_type, stroke_width);
        cap_half_width = stroke_cap_half_width(cap_type, stroke_width);
    } else if (!has_next && endpoint_end) {
        tangent_offset = stroke_cap_extension(cap_type, stroke_width);
        cap_half_width = stroke_cap_half_width(cap_type, stroke_width);
    }

    let pixel = curr_px + normal * side * cap_half_width + tangent * tangent_offset;

    var output: VertexOut;
    output.position = pixel_to_clip(pixel, curr_clip.z / max(abs(curr_clip.w), 1e-6));
    output.color = input.color;
    output.bevel_distance = vec2f(-half_width, -half_width);
    if (has_prev && has_next) {
        let turn = dir_in.x * dir_out.y - dir_in.y * dir_out.x;
        let outer_side = select(1.0, -1.0, turn > 0.0);
        let bevel_start = curr_px + normal_in * outer_side * half_width;
        let bevel_end = curr_px + normal_out * outer_side * half_width;
        let bevel_distance = side * outer_side * line_distance(bevel_start, bevel_end, pixel);
        output.bevel_distance = vec2f(bevel_distance, bevel_distance);
    }
    if (has_prev && has_next && (join_type == 1 || join_type == 2)) {
        let segment_start_px = select(curr_px, prev_px, endpoint_end);
        output.coord = vec2f(dot(pixel - segment_start_px, tangent), dot(pixel - curr_px, segment_normal));
    } else {
        output.coord = vec2f(along + tangent_offset, side * cap_half_width);
    }
    output.length_px = length_px;
    output.line_width = stroke_width;
    output.has_prev = select(0.0, 1.0, has_prev);
    output.has_next = select(0.0, 1.0, has_next);
    return output;
}
