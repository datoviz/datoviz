#ifndef DVZ_SELECTION_STYLE_GLSL
#define DVZ_SELECTION_STYLE_GLSL

#include "scene_material.glsl"

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

ItemStateStyle selectedItemStyle()
{
    return ItemStateStyle(
        uint(material.standardParams.x + 0.5), material.standardParams.y,
        material.standardParams.z, material.standardParams.w, material.emissiveRim.rgb);
}

ItemStateStyle unselectedItemStyle()
{
    return ItemStateStyle(
        uint(material.depthCue.x + 0.5), material.depthCue.y, material.depthCue.z,
        material.depthCue.w, material.depthCueColor.rgb);
}

ItemStateStyle hoveredItemStyle()
{
    return ItemStateStyle(
        uint(material.depthCueExtra.x + 0.5), material.depthCueExtra.y,
        material.depthCueExtra.z, material.depthCueExtra.w, material.model.rgb);
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
