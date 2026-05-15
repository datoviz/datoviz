struct MVP {
    model: mat4x4f,
    view: mat4x4f,
    proj: mat4x4f,
    time: f32,
    flags: u32,
};

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) size: f32,
};

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
};

@group(0) @binding(0) var<uniform> mvp: MVP;

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
    let center = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    let radius = 0.5 * input.size / 320.0;

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * radius * center.w, center.zw);
    output.color = input.color;
    output.corner = corner;
    return output;
}
