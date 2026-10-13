#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) identity: vec2u,
    @location(2) size: f32,
}

struct VertexOut {
    @location(1) alpha: f32,
    @builtin(position) position: vec4f,
    @location(0) @interpolate(flat) id: u32,
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
    let center = transform(input.position);
    let radius = vec2f(input.size / viewport.rect.z, input.size / viewport.rect.w);

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * radius * center.w, center.zw);
    output.id = input.identity.x;
    output.alpha = bitcast<f32>(input.identity.y);
    return output;
}
