#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) size: f32,
    @location(3) angle: f32,
    @location(4) shape: u32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) sprite: vec2f,
    @location(2) size: f32,
    @location(3) angle: f32,
    @location(4) @interpolate(flat) shape: u32,
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
fn main(@builtin(vertex_index) vertex_id: u32, input: VertexIn) -> VertexOut {
    let corner = quad_corner(vertex_id);
    let sprite_scale = max(abs(cos(input.angle)) + abs(sin(input.angle)), 1.0);
    let center = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    let radius = vec2f(input.size * sprite_scale / viewport.rect.z, input.size * sprite_scale / viewport.rect.w);

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * radius * center.w, center.zw);
    output.color = input.color;
    output.sprite = corner * sprite_scale;
    output.size = max(input.size, 1.0);
    output.angle = input.angle;
    output.shape = input.shape;
    return output;
}
