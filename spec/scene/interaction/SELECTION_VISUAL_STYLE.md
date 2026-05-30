# Selection Visual Style

Status: implemented v0.4 point-like slice.

## Goal

Separate retained selection state from selection presentation. `DvzSelection` should remain the
semantic source of selected item ids, link keys, and add/toggle/replace behavior, but examples and
applications must be able to choose how selected and unselected items render.

## Model

A selection owns one visual style with two independent item-state styles:

1. `selected`: applied to selected items while the selection is non-empty.
2. `unselected`: applied to non-selected items while the selection is non-empty.

When the selection is empty, neither style is applied and all items render normally.

Use flag bits on each item-state style rather than a combined mode enum. This keeps effects
composable and avoids a combinatorial set of modes as new effects appear.

Initial item-state flags:

| Flag | Meaning | v0.4 status |
| --- | --- | --- |
| `DVZ_ITEM_STATE_VISUAL_ALPHA` | Multiply item alpha by `alpha`. | Implemented for point/pixel/marker. |
| `DVZ_ITEM_STATE_VISUAL_TINT` | Mix item color toward `tint` by `tint_mix`. | Implemented for point/pixel/marker. |
| `DVZ_ITEM_STATE_VISUAL_SCALE` | Multiply point-like item size by `scale`. | Implemented for point/pixel/marker. |

Reserved later flags may include outline, hide, desaturate, depth raise, halo, pulse, and
selection-group color. Do not add these to the public API until they have at least one visual-family
implementation.

## Public API

Expose defaults and mutation through the selection API:

```c
typedef enum DvzItemStateVisualFlag
{
    DVZ_ITEM_STATE_VISUAL_NONE = 0,
    DVZ_ITEM_STATE_VISUAL_ALPHA = 1u << 0,
    DVZ_ITEM_STATE_VISUAL_TINT = 1u << 1,
    DVZ_ITEM_STATE_VISUAL_SCALE = 1u << 2,
} DvzItemStateVisualFlag;

typedef struct DvzItemStateVisualStyle
{
    uint32_t flags;
    float alpha;
    DvzColor tint;
    float tint_mix;
    float scale;
} DvzItemStateVisualStyle;

typedef struct DvzSelectionVisualStyle
{
    DvzItemStateVisualStyle selected;
    DvzItemStateVisualStyle unselected;
} DvzSelectionVisualStyle;

DvzSelectionVisualStyle dvz_selection_visual_style(void);
int dvz_selection_set_visual_style(DvzSelection* selection, const DvzSelectionVisualStyle* style);
```

Default style preserves current behavior:

- selected: no override;
- unselected: alpha dim, initially `0.25`.

A style setter resynchronizes existing `item_state_style` uniforms so changing style after a
selection exists updates the next frame.

## v0.4 Implementation Scope

Implement only the point-like retained selection shader path:

1. `dvz_point()`
2. `dvz_pixel()`
3. `dvz_marker()`

These visuals share point-like item-state plumbing and shader variants, so they are the right first
slice. Other visuals should keep current behavior or report unsupported until their selection/readout
model is explicit.

Where possible, share shader helper code between point-like item-state shaders so selected,
unselected, and hover state logic is not duplicated.

## Marker Picking Example Target

`examples/c/features/pick_marker.c` should use regular `DvzSelection` with:

- toggle selection mode;
- selected style: amber tint with full mix;
- unselected style: no flags, so other markers are not dimmed.

This proves semantic selection and configurable presentation without mutating the marker color data
outside the retained selection system.

## Non-Goals

1. Full visual-family selection styling.
2. Per-visual style overrides.
3. Animated effects, halos, outlines, or z-bias.
4. Python/high-level plotting API design.
