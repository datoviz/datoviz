#include "common.wgsl"

struct VertexIn {
    @location(0) anchor: vec3f,
    @location(1) bounds: vec4f,
    @location(2) uv_bounds: vec4f,
    @location(3) color: vec4f,
    @location(4) angle: f32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
}

fn corner(vertex_index: u32) -> vec2f {
    let k = vertex_index % 6u;
    if (k == 0u) { return vec2f(0.0, 0.0); }
    if (k == 1u) { return vec2f(0.0, 1.0); }
    if (k == 2u) { return vec2f(1.0, 0.0); }
    if (k == 3u) { return vec2f(1.0, 0.0); }
    if (k == 4u) { return vec2f(0.0, 1.0); }
    return vec2f(1.0, 1.0);
}

fn local_pixel_delta(input: VertexIn, local: vec2f) -> vec2f {
    let c = cos(input.angle);
    let s = sin(input.angle);
    let rotated = vec2f(c * local.x - s * local.y, s * local.x + c * local.y);
    return vec2f(
        select(0.0, 2.0 * rotated.x / viewport.rect.z, viewport.rect.z > 0.0),
        select(0.0, -2.0 * rotated.y / viewport.rect.w, viewport.rect.w > 0.0));
}

@vertex
fn main(input: VertexIn, @builtin(vertex_index) vertex_index: u32) -> VertexOut {
    var output: VertexOut;
    let k = corner(vertex_index);
    let local = mix(input.bounds.xy, input.bounds.zw, k);
    let uv = mix(input.uv_bounds.xy, input.uv_bounds.zw, k);
    output.position = transform(input.anchor);
    output.position.xy += local_pixel_delta(input, local) * output.position.w;
    output.uv = uv;
    output.color = input.color;
    return output;
}
