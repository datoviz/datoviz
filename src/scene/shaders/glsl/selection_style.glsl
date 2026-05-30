#ifndef DVZ_SELECTION_STYLE_GLSL
#define DVZ_SELECTION_STYLE_GLSL

#include "scene_material.glsl"

const uint DVZ_ITEM_STATE_VISUAL_ALPHA = 1u;
const uint DVZ_ITEM_STATE_VISUAL_TINT = 2u;

vec4 applySelectionVisualStyle(vec4 color, uint selection)
{
    bool selected = selection != 0u;
    uint flags = selected ? uint(material.standardParams.x + 0.5)
                          : uint(material.standardParams.w + 0.5);
    float alpha = selected ? material.standardParams.y : material.depthCue.x;
    float tintMix = selected ? material.standardParams.z : material.depthCue.y;
    vec3 tint = selected ? material.emissiveRim.rgb : material.depthCueColor.rgb;

    if ((flags & DVZ_ITEM_STATE_VISUAL_ALPHA) != 0u)
        color.a *= clamp(alpha, 0.0, 1.0);
    if ((flags & DVZ_ITEM_STATE_VISUAL_TINT) != 0u)
        color.rgb = mix(color.rgb, tint, clamp(tintMix, 0.0, 1.0));
    return color;
}

#endif
