#include "color.wgsl"

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

const LABELS_FLAG_SELECTED: u32 = 0x01u;
const LABELS_FLAG_BOUNDARY: u32 = 0x02u;

fn hashLabel(value: u32) -> u32 {
    var x = value;
    x = x ^ (x >> 16u);
    x = x * 0x7feb352du;
    x = x ^ (x >> 15u);
    x = x * 0x846ca68bu;
    x = x ^ (x >> 16u);
    return x;
}

fn labelColor(id: u32) -> vec4f {
    let count = min(labels.label_lookup[0].x, 64u);
    for (var i = 1u; i <= count; i = i + 1u) {
        if labels.label_lookup[i].x == id {
            let rgba = labels.label_lookup[i].y;
            return vec4f(
                f32((rgba >> 0u) & 0xffu),
                f32((rgba >> 8u) & 0xffu),
                f32((rgba >> 16u) & 0xffu),
                f32((rgba >> 24u) & 0xffu)) / 255.0;
        }
    }
    let h = hashLabel(id ^ labels.params.y);
    let rgb = vec3f(
        f32((h >> 0u) & 0xffu),
        f32((h >> 8u) & 0xffu),
        f32((h >> 16u) & 0xffu)) / 255.0;
    return vec4f(mix(vec3f(0.18), rgb, vec3f(0.82)), 0.82);
}

fn isHidden(id: u32) -> bool {
    let count = min(labels.params.z, 256u);
    for (var i = 0u; i < count; i = i + 1u) {
        let bits = labels.hidden_ids[i / 4u][i % 4u];
        if id == bits {
            return true;
        }
    }
    return false;
}

fn loadLabel(coord: vec2i, extent_i: vec2i) -> u32 {
    return textureLoad(tex, clamp(coord, vec2i(0), extent_i - vec2i(1)), 0).r;
}

fn selectedBoundary(coord: vec2i, extent_i: vec2i, selected_id: u32) -> bool {
    let radius = clamp(i32(round(labels.floats.y)), 1, 16);
    for (var dy = -radius; dy <= radius; dy = dy + 1) {
        for (var dx = -radius; dx <= radius; dx = dx + 1) {
            if loadLabel(coord + vec2i(dx, dy), extent_i) != selected_id {
                return true;
            }
        }
    }
    return false;
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let extent = vec2f(textureDimensions(tex));
    let extent_i = vec2i(textureDimensions(tex));
    let coord = vec2i(clamp(floor(input.uv * extent), vec2f(0.0), extent - vec2f(1.0)));
    let id = loadLabel(coord, extent_i);
    if id == labels.ids.x || isHidden(id) {
        discard;
    }
    var color = semantic_color_to_linear(labelColor(id));
    let selected = (labels.params.x & LABELS_FLAG_SELECTED) != 0u && id == labels.ids.y;
    if selected {
        let boundary_color = semantic_color_to_linear(labels.boundary_color);
        let boundary = (labels.params.x & LABELS_FLAG_BOUNDARY) != 0u &&
            selectedBoundary(coord, extent_i, labels.ids.y);
        if boundary {
            color = boundary_color;
        } else {
            color = mix(color, boundary_color, vec4f(0.35));
        }
    }
    color.a = color.a * labels.floats.x;
    return color;
}
