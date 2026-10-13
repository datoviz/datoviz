struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) local_uv: vec2f,
}

@vertex
fn main(@builtin(vertex_index) index: u32) -> VertexOut {
    let positions = array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0));
    let position = positions[index];
    var output: VertexOut;
    output.position = vec4f(position, 0.0, 1.0);
    // Sample textures in framebuffer coordinates, whose origin is at the top left.
    output.local_uv = vec2f(position.x * 0.5 + 0.5, 0.5 - position.y * 0.5);
    return output;
}
