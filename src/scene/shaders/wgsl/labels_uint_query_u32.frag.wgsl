struct FragmentIn {
    @location(0) uv: vec2f,
}

@group(1) @binding(0) var tex: texture_2d<u32>;
@group(1) @binding(1) var samp: sampler;

struct LabelsParams {
    ids: vec4u,
    params: vec4u,
    floats: vec4f,
    boundary_color: vec4f,
    hidden_ids: array<vec4u, 64>,
    label_lookup: array<vec4u, 65>,
}

@group(1) @binding(2) var<uniform> labels: LabelsParams;

fn isHidden(id: u32) -> bool {
    let count = min(labels.params.z, 256u);
    for (var i = 0u; i < count; i = i + 1u) {
        if id == labels.hidden_ids[i / 4u][i % 4u] {
            return true;
        }
    }
    return false;
}

@fragment
fn main(input: FragmentIn) -> @location(0) u32 {
    let extent = vec2f(textureDimensions(tex));
    let extent_i = vec2i(textureDimensions(tex));
    let coord = vec2i(clamp(floor(input.uv * extent), vec2f(0.0), extent - vec2f(1.0)));
    let id = textureLoad(tex, clamp(coord, vec2i(0), extent_i - vec2i(1)), 0).r;
    if id == labels.ids.x || isHidden(id) {
        discard;
    }
    return id + 1u;
}
