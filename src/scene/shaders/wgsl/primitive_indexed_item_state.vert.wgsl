#include "common.wgsl"

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

struct ItemStateStyleParams {
    selected: vec4f,
    selected_tint: vec4f,
    unselected: vec4f,
    unselected_tint: vec4f,
    hovered: vec4f,
    hovered_tint: vec4f,
}

@group(1) @binding(1) var<uniform> item_state_style: ItemStateStyleParams;

fn selected_item_style() -> ItemStateStyle {
    return ItemStateStyle(
        u32(item_state_style.selected.x + 0.5),
        item_state_style.selected.y,
        item_state_style.selected.z,
        item_state_style.selected.w,
        item_state_style.selected_tint.rgb,
    );
}

fn unselected_item_style() -> ItemStateStyle {
    return ItemStateStyle(
        u32(item_state_style.unselected.x + 0.5),
        item_state_style.unselected.y,
        item_state_style.unselected.z,
        item_state_style.unselected.w,
        item_state_style.unselected_tint.rgb,
    );
}

fn hovered_item_style() -> ItemStateStyle {
    return ItemStateStyle(
        u32(item_state_style.hovered.x + 0.5),
        item_state_style.hovered.y,
        item_state_style.hovered.z,
        item_state_style.hovered.w,
        item_state_style.hovered_tint.rgb,
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


@group(1) @binding(2) var<storage, read> item_state_words: array<u32>;

struct VertexIn {
    @location(7) state_index: u32,
    @location(0) position: vec3f,
    @location(1) color: vec4f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    var output: VertexOut;
    output.position = transform(input.position);
    output.color = apply_item_state_color(input.color, item_state_words[input.state_index]);
    return output;
}
