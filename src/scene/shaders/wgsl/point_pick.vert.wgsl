#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) size: f32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) id: u32,
    @location(1) corner: vec2f,
    @location(2) size: f32,
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
    let center = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    let sprite_size = max(input.size + 4.0, 1.0);
    let radius = vec2f(sprite_size / viewport.rect.z, sprite_size / viewport.rect.w);

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * radius * center.w, center.zw);
    output.id = instance_id + 1u;
    output.corner = corner;
    output.size = input.size;
    return output;
}
