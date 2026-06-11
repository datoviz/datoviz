#include "common.wgsl"
#include "scene_material.wgsl"

struct VertexIn {
    @location(0) position_start: vec3f,
    @location(1) position_end: vec3f,
    @location(2) color: vec4f,
    @location(3) line_width: f32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) coord: vec2f,
    @location(2) length_px: f32,
    @location(3) line_width: f32,
}

const CLIP_EPS: f32 = 1e-5;

fn clip_to_pixel(clip: vec4f) -> vec2f {
    let ndc = clip.xy / max(clip.w, CLIP_EPS);
    return (ndc * 0.5 + vec2f(0.5)) * viewport.rect.zw;
}

fn pixel_to_clip(pixel: vec2f, clip: vec4f) -> vec4f {
    let ndc = pixel / max(viewport.rect.zw, vec2f(1.0)) * 2.0 - vec2f(1.0);
    return vec4f(ndc * clip.w, clip.z, clip.w);
}

fn clip_segment_plane(
    start_clip: ptr<function, vec4f>, end_clip: ptr<function, vec4f>, start_dist: f32,
    end_dist: f32) -> bool {
    if (start_dist < 0.0 && end_dist < 0.0) {
        return false;
    }
    if (start_dist < 0.0 || end_dist < 0.0) {
        let t = start_dist / (start_dist - end_dist);
        let clipped = mix(*start_clip, *end_clip, clamp(t, 0.0, 1.0));
        if (start_dist < 0.0) {
            *start_clip = clipped;
        } else {
            *end_clip = clipped;
        }
    }
    return true;
}

fn clip_segment_to_view(start_clip: ptr<function, vec4f>, end_clip: ptr<function, vec4f>) -> bool {
    if (!clip_segment_plane(
        start_clip, end_clip, (*start_clip).w - CLIP_EPS, (*end_clip).w - CLIP_EPS)) {
        return false;
    }
    return clip_segment_plane(start_clip, end_clip, (*start_clip).z, (*end_clip).z);
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
fn main(@builtin(vertex_index) vertex_index: u32, input: VertexIn) -> VertexOut {
    var start_clip = transform(input.position_start);
    var end_clip = transform(input.position_end);
    if (!clip_segment_to_view(&start_clip, &end_clip)) {
        var output: VertexOut;
        output.position = vec4f(2.0, 2.0, 1.0, 1.0);
        output.color = vec4f(0.0);
        output.coord = vec2f(0.0);
        output.length_px = 0.0;
        output.line_width = 0.0;
        return output;
    }

    let start_px = clip_to_pixel(start_clip);
    let end_px = clip_to_pixel(end_clip);

    var tangent = end_px - start_px;
    let length_px = length(tangent);
    if (length_px <= 1e-6) {
        tangent = vec2f(1.0, 0.0);
    } else {
        tangent = tangent / length_px;
    }
    let normal = vec2f(-tangent.y, tangent.x);

    let line_width = max(input.line_width, 0.0);
    let start_cap = i32(material.params.x + 0.5);
    let end_cap = i32(material.params.y + 0.5);
    let start_extension = stroke_cap_extension(start_cap, line_width);
    let end_extension = stroke_cap_extension(end_cap, line_width);
    let start_half_width = stroke_cap_half_width(start_cap, line_width);
    let end_half_width = stroke_cap_half_width(end_cap, line_width);
    let vertex = vertex_index & 3u;

    var pixel = start_px;
    var clip = start_clip;
    var coord = vec2f(0.0);

    if (vertex == 0u) {
        pixel = start_px - tangent * start_extension + normal * start_half_width;
        coord = vec2f(-start_extension, start_half_width);
    } else if (vertex == 1u) {
        pixel = start_px - tangent * start_extension - normal * start_half_width;
        coord = vec2f(-start_extension, -start_half_width);
    } else if (vertex == 2u) {
        pixel = end_px + tangent * end_extension - normal * end_half_width;
        coord = vec2f(length_px + end_extension, -end_half_width);
        clip = end_clip;
    } else {
        pixel = end_px + tangent * end_extension + normal * end_half_width;
        coord = vec2f(length_px + end_extension, end_half_width);
        clip = end_clip;
    }

    var output: VertexOut;
    output.position = pixel_to_clip(pixel, clip);
    output.color = input.color;
    output.coord = coord;
    output.length_px = length_px;
    output.line_width = line_width;
    return output;
}
