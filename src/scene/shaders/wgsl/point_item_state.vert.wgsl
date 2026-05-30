#include "common.wgsl"
#include "scene_material.wgsl"

const DVZ_ITEM_STATE_HOVERED: u32 = 1u;
const DVZ_ITEM_STATE_SELECTED: u32 = 2u;

const DVZ_ITEM_STATE_VISUAL_ALPHA: u32 = 1u;
const DVZ_ITEM_STATE_VISUAL_TINT: u32 = 2u;
const DVZ_ITEM_STATE_VISUAL_SCALE: u32 = 4u;

struct ItemStateStyle {
    flags: u32,
    alpha: f32,
    tint_mix: f32,
    scale: f32,
    tint: vec3f,
}

fn selected_item_style() -> ItemStateStyle {
    return ItemStateStyle(
        u32(material.standard_params.x + 0.5),
        material.standard_params.y,
        material.standard_params.z,
        material.standard_params.w,
        material.emissive_rim.rgb,
    );
}

fn unselected_item_style() -> ItemStateStyle {
    return ItemStateStyle(
        u32(material.depth_cue.x + 0.5),
        material.depth_cue.y,
        material.depth_cue.z,
        material.depth_cue.w,
        material.depth_cue_color.rgb,
    );
}

fn hovered_item_style() -> ItemStateStyle {
    return ItemStateStyle(
        u32(material.depth_cue_extra.x + 0.5),
        material.depth_cue_extra.y,
        material.depth_cue_extra.z,
        material.depth_cue_extra.w,
        material.model.rgb,
    );
}

fn apply_one_item_state_color(color: vec4f, style: ItemStateStyle) -> vec4f {
    var out = color;
    if ((style.flags & DVZ_ITEM_STATE_VISUAL_ALPHA) != 0u) {
        out.a *= clamp(style.alpha, 0.0, 1.0);
    }
    if ((style.flags & DVZ_ITEM_STATE_VISUAL_TINT) != 0u) {
        out = vec4f(mix(out.rgb, style.tint, clamp(style.tint_mix, 0.0, 1.0)), out.a);
    }
    return out;
}

fn apply_one_item_state_scale(size: f32, style: ItemStateStyle) -> f32 {
    var out = size;
    if ((style.flags & DVZ_ITEM_STATE_VISUAL_SCALE) != 0u) {
        out *= max(style.scale, 0.0);
    }
    return out;
}

fn apply_item_state_color(color: vec4f, item_state: u32) -> vec4f {
    let selected = (item_state & DVZ_ITEM_STATE_SELECTED) != 0u;
    let hovered = (item_state & DVZ_ITEM_STATE_HOVERED) != 0u;
    var out = color;
    if (!selected) {
        out = apply_one_item_state_color(out, unselected_item_style());
    }
    if (selected) {
        out = apply_one_item_state_color(out, selected_item_style());
    }
    if (hovered) {
        out = apply_one_item_state_color(out, hovered_item_style());
    }
    return out;
}

fn apply_item_state_scale(size: f32, item_state: u32) -> f32 {
    let selected = (item_state & DVZ_ITEM_STATE_SELECTED) != 0u;
    let hovered = (item_state & DVZ_ITEM_STATE_HOVERED) != 0u;
    var out = size;
    if (!selected) {
        out = apply_one_item_state_scale(out, unselected_item_style());
    }
    if (selected) {
        out = apply_one_item_state_scale(out, selected_item_style());
    }
    if (hovered) {
        out = apply_one_item_state_scale(out, hovered_item_style());
    }
    return out;
}

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) size: f32,
    @location(5) item_state: u32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
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
    let center = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);
    let size = apply_item_state_scale(input.size, input.item_state);
    let radius = vec2f(size / viewport.rect.z, size / viewport.rect.w);

    var output: VertexOut;
    output.position = vec4f(center.xy + corner * radius * center.w, center.zw);
    output.color = apply_item_state_color(input.color, input.item_state);
    output.corner = corner;
    return output;
}
