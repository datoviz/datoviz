#ifndef DVZ_SELECTION_STYLE_GLSL
#define DVZ_SELECTION_STYLE_GLSL

const uint DVZ_ITEM_STATE_HOVERED = 1u;
const uint DVZ_ITEM_STATE_SELECTED = 2u;

const uint DVZ_ITEM_STATE_VISUAL_ALPHA = 1u;
const uint DVZ_ITEM_STATE_VISUAL_TINT = 2u;
const uint DVZ_ITEM_STATE_VISUAL_SCALE = 4u;

struct ItemStateStyle {
    uint flags;
    float alpha;
    float tintMix;
    float scale;
    vec3 tint;
};

layout(set = 1, binding = 1) uniform ItemStateStyleParams {
    vec4 selected;
    vec4 selectedTint;
    vec4 unselected;
    vec4 unselectedTint;
    vec4 hovered;
    vec4 hoveredTint;
} itemStateStyle;

ItemStateStyle selectedItemStyle()
{
    return ItemStateStyle(
        uint(itemStateStyle.selected.x + 0.5), itemStateStyle.selected.y,
        itemStateStyle.selected.z, itemStateStyle.selected.w, itemStateStyle.selectedTint.rgb);
}

ItemStateStyle unselectedItemStyle()
{
    return ItemStateStyle(
        uint(itemStateStyle.unselected.x + 0.5), itemStateStyle.unselected.y,
        itemStateStyle.unselected.z, itemStateStyle.unselected.w,
        itemStateStyle.unselectedTint.rgb);
}

ItemStateStyle hoveredItemStyle()
{
    return ItemStateStyle(
        uint(itemStateStyle.hovered.x + 0.5), itemStateStyle.hovered.y,
        itemStateStyle.hovered.z, itemStateStyle.hovered.w, itemStateStyle.hoveredTint.rgb);
}

vec4 applyOneItemStateColor(vec4 color, ItemStateStyle style)
{
    if ((style.flags & DVZ_ITEM_STATE_VISUAL_ALPHA) != 0u)
        color.a *= clamp(style.alpha, 0.0, 1.0);
    if ((style.flags & DVZ_ITEM_STATE_VISUAL_TINT) != 0u)
        color.rgb = mix(color.rgb, style.tint, clamp(style.tintMix, 0.0, 1.0));
    return color;
}

float applyOneItemStateScale(float size, ItemStateStyle style)
{
    if ((style.flags & DVZ_ITEM_STATE_VISUAL_SCALE) != 0u)
        size *= max(style.scale, 0.0);
    return size;
}

vec4 applyItemStateColor(vec4 color, uint itemState)
{
    bool selected = (itemState & DVZ_ITEM_STATE_SELECTED) != 0u;
    bool hovered = (itemState & DVZ_ITEM_STATE_HOVERED) != 0u;
    if (!selected)
        color = applyOneItemStateColor(color, unselectedItemStyle());
    if (selected)
        color = applyOneItemStateColor(color, selectedItemStyle());
    if (hovered)
        color = applyOneItemStateColor(color, hoveredItemStyle());
    return color;
}

float applyItemStateScale(float size, uint itemState)
{
    bool selected = (itemState & DVZ_ITEM_STATE_SELECTED) != 0u;
    bool hovered = (itemState & DVZ_ITEM_STATE_HOVERED) != 0u;
    if (!selected)
        size = applyOneItemStateScale(size, unselectedItemStyle());
    if (selected)
        size = applyOneItemStateScale(size, selectedItemStyle());
    if (hovered)
        size = applyOneItemStateScale(size, hoveredItemStyle());
    return size;
}

#endif
