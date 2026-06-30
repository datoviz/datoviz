// datoviz-builtin-shader: scene.primitive lit_instanced_item_state vertex v1

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

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) normal: vec3f,
    @location(3) instance_transform0: vec4f,
    @location(4) instance_transform1: vec4f,
    @location(5) instance_transform2: vec4f,
    @location(6) instance_transform3: vec4f,
    @location(7) item_state: u32,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) world_position: vec3f,
    @location(3) camera_position: vec3f,
    @location(4) depth: f32,
}

fn instance_transform(input: VertexIn) -> mat4x4f {
    return mat4x4f(
        input.instance_transform0,
        input.instance_transform1,
        input.instance_transform2,
        input.instance_transform3,
    );
}

@vertex
fn main(input: VertexIn) -> VertexOut {
    let model = mvp.model * instance_transform(input);
    let scale = apply_item_state_scale(1.0, input.item_state);
    let local = vec4f(input.position * scale, 1.0);
    let world = model * local;
    let clip = mvp.proj * mvp.view * world;

    var output: VertexOut;
    output.position = clip;
    output.color = apply_item_state_color(input.color, input.item_state);
    output.normal = transpose(inverse(mat3x3f(
        model[0].xyz,
        model[1].xyz,
        model[2].xyz,
    ))) * input.normal;
    output.world_position = world.xyz;
    output.camera_position = camera_position_from_view();
    output.depth = clip.z / max(abs(clip.w), 1e-6);
    return output;
}
