#include "common.wgsl"

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

fn image_pixel_anchor(position: vec3f) -> vec4f {
    let size = max(viewport.rect.zw, vec2f(1.0, 1.0));
    let ndc = vec2f(-1.0 + 2.0 * position.x / size.x,
                    1.0 - 2.0 * position.y / size.y);
    return vec4f(ndc, position.z, 1.0);
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = image_pixel_anchor(input.position);
    output.uv = input.uv;
    return output;
}
