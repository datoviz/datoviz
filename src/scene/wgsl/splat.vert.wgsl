#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) sigma: vec2f,
    @location(3) angle: f32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) offset_px: vec2f,
    @location(2) sigma: vec2f,
    @location(3) angle: f32,
}

const CUTOFF_SIGMA: f32 = 3.0;

fn quad_corner(vertex_id: u32) -> vec2f {
    let corners = array<vec2f, 6>(
        vec2f(-1.0, -1.0),
        vec2f( 1.0, -1.0),
        vec2f(-1.0,  1.0),
        vec2f(-1.0,  1.0),
        vec2f( 1.0, -1.0),
        vec2f( 1.0,  1.0),
    );
    return corners[vertex_id % 6u];
}

@vertex
fn main(@builtin(vertex_index) vertex_id: u32, input: VertexIn) -> VertexOut {
    let sigma = max(input.sigma, vec2f(0.000001));
    let extent = CUTOFF_SIGMA * max(sigma.x, sigma.y);
    let corner = quad_corner(vertex_id);
    let center = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    let viewport_size = max(viewport.rect.zw, vec2f(1.0));
    let ndc_radius = 2.0 * vec2f(extent / viewport_size.x, extent / viewport_size.y);

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * ndc_radius * center.w, center.zw);
    output.color = input.color;
    output.offset_px = corner * extent;
    output.sigma = sigma;
    output.angle = input.angle;
    return output;
}
