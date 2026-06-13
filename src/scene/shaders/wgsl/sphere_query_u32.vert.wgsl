#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) radius: f32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) center_view: vec4f,
    @location(2) radius: f32,
    @location(3) sprite_scale: f32,
    @location(4) @interpolate(flat) id: u32,
    @location(5) coord: vec2f,
}

fn transform_radius(radius: f32) -> f32 {
    let sx = length(mvp.model[0].xyz);
    let sy = length(mvp.model[1].xyz);
    let sz = length(mvp.model[2].xyz);
    return radius * max(max(sx, sy), sz);
}

fn quad_corner(vertex_id: u32) -> vec2f {
    let corners = array<vec2f, 6>(
        vec2f(-1.0, -1.0),
        vec2f( 1.0, -1.0),
        vec2f(-1.0,  1.0),
        vec2f(-1.0,  1.0),
        vec2f( 1.0, -1.0),
        vec2f( 1.0,  1.0),
    );
    return corners[vertex_id];
}

@vertex
fn main(
    @builtin(instance_index) instance_id: u32,
    @builtin(vertex_index) vertex_id: u32,
    input: VertexIn,
) -> VertexOut {
    let corner = quad_corner(vertex_id);
    let center_view = mvp.view * mvp.model * vec4f(input.position, 1.0);
    let center = transform(input.position);
    let radius = max(transform_radius(input.radius), 1e-6);
    let inv_w = 1.0 / max(abs(center.w), 1e-6);
    let ndc_radius_data = vec2f(
        radius * abs(mvp.proj[0][0]) * inv_w,
        radius * abs(mvp.proj[1][1]) * inv_w,
    );
    let radius_px = 0.5 * max(
        ndc_radius_data.x * viewport.rect.z,
        ndc_radius_data.y * viewport.rect.w,
    );
    let padded_radius_px = radius_px + 1.5;
    let ndc_radius = ndc_radius_data + vec2f(
        3.0 / max(viewport.rect.z, 1.0),
        3.0 / max(viewport.rect.w, 1.0),
    );

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * ndc_radius * center.w, center.zw);
    output.color = input.color;
    output.center_view = center_view;
    output.radius = radius;
    output.sprite_scale = padded_radius_px / max(radius_px, 1e-6);
    output.id = instance_id + 1u;
    output.coord = corner * output.sprite_scale;
    return output;
}
