# Item State System

Status: planned architecture, first v0.4 implementation slice active.

## Goal

Use one retained item-state model for hover, selection, focus/active items, linked highlights,
filtering, and disabled items. Avoid separate ad-hoc visual mutation paths for each interaction.

The long-term flow is:

```text
panel query -> interaction state writer -> per-item state bitfield -> visual state style shader
```

## State Bits

Each queryable item may carry an `item_state` bitfield. Initial public bits:

| Bit | Meaning | Initial writer |
| --- | --- | --- |
| `DVZ_ITEM_STATE_HOVERED` | transient item under the cursor or current hover query | `DvzHover` |
| `DVZ_ITEM_STATE_SELECTED` | persistent selected item | `DvzSelection` |
| `DVZ_ITEM_STATE_ACTIVE` | active/dragged/current item | future |
| `DVZ_ITEM_STATE_LINKED` | linked item/group highlight | future |
| `DVZ_ITEM_STATE_FILTERED` | item outside an active filter/focus | future |
| `DVZ_ITEM_STATE_DISABLED` | item is present but visually inactive | future |

`unselected` is not a state bit. It is contextual selection presentation applied only while a
selection is non-empty and the item lacks `DVZ_ITEM_STATE_SELECTED`.

## Style Model

`DvzItemStateVisualStyle` is the reusable visual-effect descriptor. It is used by hover and by the
selected/unselected halves of `DvzSelectionVisualStyle`.

Initial visual effects:

| Flag | Meaning | First visual support |
| --- | --- | --- |
| `DVZ_ITEM_STATE_VISUAL_ALPHA` | multiply item alpha by `alpha` | point, marker |
| `DVZ_ITEM_STATE_VISUAL_TINT` | mix item color toward `tint` by `tint_mix` | point, marker |
| `DVZ_ITEM_STATE_VISUAL_SCALE` | multiply point-like item size by `scale` | point, marker |

Future effects may include outline, hide, desaturate, z-bias/raise, halo, pulse, and per-selection
palette/group color. Add public flags only when at least one visual family implements them.

## Writers

### Selection

`DvzSelection` owns persistent selected targets and writes `DVZ_ITEM_STATE_SELECTED` into the
combined item-state field. It also owns contextual visual style:

```c
DvzSelectionVisualStyle {
    DvzItemStateVisualStyle selected;
    DvzItemStateVisualStyle unselected;
}
```

Default selection style preserves legacy behavior: selected normal, unselected alpha-dimmed.

### Hover

`DvzHover` owns one transient latest-hit item and writes `DVZ_ITEM_STATE_HOVERED`. It owns a single
`DvzItemStateVisualStyle` because hover has no contextual inverse.

Examples should use `DvzHover` instead of manually mutating visual attributes for hover feedback.

## Composition Order

Shaders should apply styles deterministically:

1. unselected selection context, when a selection is active and item is not selected;
2. selected style;
3. linked style, later;
4. hovered style;
5. active/dragged style, later;
6. disabled/filter rules, once designed.

Hover is applied after selection so hover feedback remains visible on selected items.

## v0.4 Slice

Implement only retained point-like item state for:

1. `dvz_point()`
2. `dvz_marker()`

The retained visual attribute is `item_state` (`uint32_t` per item). Point/marker selection shader
variants consume this bitfield and shared style uniforms. Other visual families keep their current
behavior until their item semantics are explicit.

## Non-Goals For First Slice

1. Mesh face/region state.
2. Path/subpath state.
3. Image/volume probe-state presentation.
4. Per-visual style overrides.
5. Multiple simultaneous selections/hovers with independent styles. The first active writer style
   wins until a composition policy is required.

## Convenience Panel Controller

`DvzHover` and `DvzSelection` remain the semantic state objects. They are intentionally separate even
though both write the same `item_state` field: hover is transient pointer/query state, while
selection is persistent application state with selection modes and optional metadata UI.

For normal user code, add one convenience controller bound to a panel:

```c
DvzItemInteraction* dvz_item_interaction(
    DvzPanel* panel, const DvzItemInteractionDesc* desc);

DvzItemInteractionDesc dvz_item_interaction_desc(void);
void dvz_item_interaction_destroy(DvzItemInteraction* interaction);
DvzHover* dvz_item_interaction_hover(DvzItemInteraction* interaction);
DvzSelection* dvz_item_interaction_selection(DvzItemInteraction* interaction);
```

The default call should be useful:

```c
DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
```

Default behavior:

1. query `DVZ_SCENE_TARGET_ITEM` with `DVZ_QUERY_HIT_FRONTMOST`;
2. update hover on pointer move and clear it on miss/leave;
3. toggle selection on left-button click;
4. clear selection on background click;
5. create default `DvzHover` and `DvzSelection` objects when the descriptor does not provide them.

Descriptor fields should stay small and semantic:

```c
struct DvzItemInteractionDesc
{
    DvzHover* hover;           // optional externally-owned/shared hover object
    DvzSelection* selection;   // optional externally-owned/shared selection object
    bool hover_enabled;
    bool selection_enabled;
    DvzSelectMode select_mode;
    DvzSceneTargetKind target;
    DvzQueryHitPolicy hit_policy;
    bool clear_hover_on_miss;
    bool clear_selection_on_miss;
};
```

Examples:

```c
// Defaults: hover + toggle selection.
DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
```

```c
// Additive selection, still with hover.
DvzItemInteraction* pick = dvz_item_interaction(panel, &(DvzItemInteractionDesc){
    .select_mode = DVZ_SELECT_ADDITIVE,
});
```

```c
// Hover only.
DvzItemInteraction* pick = dvz_item_interaction(panel, &(DvzItemInteractionDesc){
    .selection_enabled = false,
});
```

```c
// Selection only.
DvzItemInteraction* pick = dvz_item_interaction(panel, &(DvzItemInteractionDesc){
    .hover_enabled = false,
});
```

```c
// Customize generated state objects.
DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
DvzHover* hover = dvz_item_interaction_hover(pick);
DvzSelection* selection = dvz_item_interaction_selection(pick);
```

Do not add `dvz_item_hover()` or `dvz_item_selection()` convenience constructors. They blur the
semantic boundary without removing meaningful boilerplate. Users who need explicit state objects can
call `dvz_hover()` and `dvz_selection()` directly, then pass them to `dvz_item_interaction()`.
